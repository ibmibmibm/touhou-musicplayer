/**
 * This file is part of Touhou Music Player.
 *
 * Touhou Music Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Touhou Music Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Touhou Music Player.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "musicfile.h"

MusicFile::MusicFile(const QString& fileName) :
    _fileName(fileName),
    _fileEngine(QAbstractFileEngine::create(_fileName))
{
    //qDebug() << Q_FUNC_INFO << fileName;
}

MusicFile::MusicFile(const MusicData& fileDescription) :
    _loop(fileDescription.loop()),
    _loopBegin(fileDescription.loopBegin()),
    _loopEnd(fileDescription.loopEnd()),
    _fileName(fileDescription.fileName()),
    _fileEngine(QAbstractFileEngine::create(_fileName))
{
    //qDebug() << _fileName;
    //qDebug() << Q_FUNC_INFO << fileDescription.fileName << fileDescription.loop << fileDescription.loopBegin << fileDescription.loopEnd;
    if (fileDescription.archiveMusicData().data() != NULL)
    {
        _archiveMusicData = fileDescription.archiveMusicData();
        setFileName(_archiveMusicData->archiveFileName);
    }
}

const QString& MusicFile::fileName() const
{
    return _fileName;
}

void MusicFile::setFileName(const QString& fileName)
{
    //qDebug() << Q_FUNC_INFO
    delete _fileEngine;
    _fileEngine = QAbstractFileEngine::create(fileName);
    _fileName = fileName;
}

void MusicFile::setFileDescription(const MusicData& fileDescription)
{
    //qDebug() << Q_FUNC_INFO
    setFileName(fileDescription.fileName());
    _loop = fileDescription.loop();
    _loopBegin = fileDescription.loopBegin();
    _loopEnd = fileDescription.loopEnd();
    if (fileDescription.archiveMusicData().data() != NULL)
    {
        _archiveMusicData = fileDescription.archiveMusicData();
        setFileName(_archiveMusicData->archiveFileName);
    }
}

bool MusicFile::open(OpenMode mode)
{
    //qDebug() << Q_FUNC_INFO << mode << static_cast<int>(mode);
    Q_ASSERT(!(mode & QIODevice::ReadOnly) || !(mode & QIODevice::Text));
    Q_ASSERT(!(mode & QIODevice::Text));
    Q_ASSERT(!(mode & QIODevice::Append));
    return _fileEngine->open(mode) && QIODevice::open(mode);
}

qint64 MusicFile::pos() const
{
    //qDebug() << Q_FUNC_INFO;
    return _pos();
}

qint64 MusicFile::_pos() const
{
    if (_archiveMusicData.data() == NULL)
    {
        //qDebug() << Q_FUNC_INFO << "pos" << _fileEngine->pos();
        return _fileEngine->pos();
    }
    //qDebug() << Q_FUNC_INFO << "pos" << _fileEngine->pos() - _archiveMusicData->dataBegin;
    return _fileEngine->pos() - _archiveMusicData->dataBegin;
}

qint64 MusicFile::size() const
{
    //qDebug() << Q_FUNC_INFO;
    return _size();
}

qint64 MusicFile::_size() const
{
    //qDebug() << Q_FUNC_INFO << "_fileEngine->size()" << _fileEngine->size();
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->size();
    return _archiveMusicData->dataEnd - _archiveMusicData->dataBegin;
}

bool MusicFile::seek(qint64 pos)
{
    //qDebug() << Q_FUNC_INFO;
    return _seek(pos);
}

bool MusicFile::_seek(qint64 pos)
{
    //qDebug() << Q_FUNC_INFO << "pos" << pos;
    Q_ASSERT(pos >= 0);
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->seek(pos);
    Q_ASSERT(pos <= _archiveMusicData->dataEnd - _archiveMusicData->dataBegin);
    //qDebug() << Q_FUNC_INFO << "adjected" << _archiveMusicData->dataBegin + pos;
    return _fileEngine->seek(_archiveMusicData->dataBegin + pos);
}

bool MusicFile::reset()
{
    //qDebug() << Q_FUNC_INFO;
    return _reset();
}

bool MusicFile::_reset()
{
    //qDebug() << Q_FUNC_INFO;
    bool result = QIODevice::reset();
    if (_archiveMusicData.data() == NULL)
        result &= _fileEngine->seek(0);
    else
        result &= _fileEngine->seek(_archiveMusicData->dataBegin);
    return result;
}

qint64 MusicFile::readData(char* data, qint64 maxSize)
{
    return _readData(data, maxSize);
}

qint64 MusicFile::_readData(char* data, qint64 maxSize)
{
    //qDebug() << Q_FUNC_INFO << "maxSize" << maxSize;
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->read(data, maxSize);
    //qDebug() << Q_FUNC_INFO << "_fileEngine->pos()" << _fileEngine->pos() << "_archiveMusicData->dataEnd" << _archiveMusicData->dataEnd;
    if (maxSize > _archiveMusicData->dataEnd - _fileEngine->pos())
        maxSize = _archiveMusicData->dataEnd - _fileEngine->pos();
    //qDebug() << Q_FUNC_INFO << "adjected" << maxSize;
    if (maxSize <= 0)
        return 0;
    if (_archiveMusicData->decoder == NULL)
        return _fileEngine->read(data, maxSize);
    qint64 result = _fileEngine->read(data, maxSize);
    for (qint64 i = 0; i < result; ++i)
    {
        data[i] = _archiveMusicData->decoder(_archiveMusicData->userData, data[i]);
    }
    //qDebug() << Q_FUNC_INFO << "result" << result;
    return result;
}

qint64 MusicFile::writeData(const char* data, qint64 maxSize)
{
    return _writeData(data, maxSize);
}

qint64 MusicFile::_writeData(const char* data, qint64 maxSize)
{
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->write(data, maxSize);
    if (maxSize > _archiveMusicData->dataEnd - _fileEngine->pos())
        maxSize = _archiveMusicData->dataEnd - _fileEngine->pos();
    if (_archiveMusicData->encoder == NULL)
        return _fileEngine->write(data, maxSize);
    return _fileEngine->write(data, maxSize);
}
