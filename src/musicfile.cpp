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
#include <QFSFileEngine>
#include <QtDebug>
#include "musicfile.h"

MusicFile::MusicFile(const MusicData& fileDescription) :
    _loop(fileDescription.loop()),
    _loopBegin(fileDescription.loopBegin()),
    _loopEnd(fileDescription.loopEnd()),
    _title(fileDescription.title()),
    _album(fileDescription.album()),
    _fileName(fileDescription.fileName()),
    _fileEngine(QAbstractFileEngine::create(_fileName))
{
    if (fileDescription.archiveMusicData().data() == NULL)
        return;
    _archiveMusicData = fileDescription.archiveMusicData();
    setFileName(_archiveMusicData->archiveFileName);
}

const QString& MusicFile::fileName() const
{
    return _fileName;
}

void MusicFile::setFileName(const QString& fileName)
{
    delete _fileEngine;
    _fileEngine = QAbstractFileEngine::create(fileName);
    _fileName = fileName;
}

void MusicFile::setFileDescription(const MusicData& fileDescription)
{
    setFileName(fileDescription.fileName());
    _loop = fileDescription.loop();
    _loopBegin = fileDescription.loopBegin();
    _loopEnd = fileDescription.loopEnd();
    if (fileDescription.archiveMusicData().data() == NULL)
        return;
    _archiveMusicData = fileDescription.archiveMusicData();
    setFileName(_archiveMusicData->archiveFileName);
}

bool MusicFile::open(OpenMode mode)
{
    Q_ASSERT(!(mode & QIODevice::ReadOnly) || !(mode & QIODevice::Text));
    Q_ASSERT(!(mode & QIODevice::Text));
    Q_ASSERT(!(mode & QIODevice::Append));
    return _fileEngine->open(mode) && QIODevice::open(mode);
}

qint64 MusicFile::_pos() const
{
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->pos();
    return _fileEngine->pos() - _archiveMusicData->dataBegin;
}

qint64 MusicFile::size() const
{
    return _size();
}

qint64 MusicFile::_size() const
{
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->size();
    return _archiveMusicData->dataEnd - _archiveMusicData->dataBegin;
}

bool MusicFile::seek(qint64 pos)
{
    return QIODevice::seek(pos) && _seek(pos);
}

bool MusicFile::_seek(qint64 pos)
{
    if (pos < 0)
    {
        qWarning() << Q_FUNC_INFO << ": try to seek to " << pos << ", a negative position.";
        return false;
    }
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->seek(pos);
    if (pos > _archiveMusicData->dataEnd - _archiveMusicData->dataBegin)
    {
        qWarning() << Q_FUNC_INFO << ": try to seek to " << pos << ", a invalid position.";
        return false;
    }
    return _fileEngine->seek(_archiveMusicData->dataBegin + pos);
}

bool MusicFile::reset()
{
    return _reset();
}

bool MusicFile::_reset()
{
    return QIODevice::reset() & _fileEngine->seek((_archiveMusicData.data() == NULL) ? 0 : _archiveMusicData->dataBegin);
}

qint64 MusicFile::readData(char* data, qint64 maxSize)
{
    return _readData(data, maxSize);
}

qint64 MusicFile::_readData(char* data, qint64 maxSize)
{
    if (_archiveMusicData.data() == NULL)
        return _fileEngine->read(data, maxSize);
    qint64 realMaxSize = qMin(maxSize, _archiveMusicData->dataEnd - _fileEngine->pos());
    if (realMaxSize <= 0)
        return 0;
    if (_archiveMusicData->decoder == NULL)
        return _fileEngine->read(data, realMaxSize);
    qint64 result = _fileEngine->read(data, realMaxSize);
    for (qint64 i = 0; i < result; ++i)
        data[i] = _archiveMusicData->decoder(_archiveMusicData->userData, data[i]);
    return result;
}


QHash<QString, MusicFileFactory::CreateFunction> MusicFileFactory::functionHash;

int MusicFileFactory::registerMusicFile(const QString& suffix, CreateFunction createFunction)
{
    functionHash.insert(suffix, createFunction);
    return functionHash.size();
}

MusicFile* MusicFileFactory::createMusicFile(const MusicData& fileDescription)
{
    const QString& suffix = fileDescription.suffix();
    Q_ASSERT(functionHash.contains(suffix));
    return functionHash.value(suffix)(fileDescription);
}
