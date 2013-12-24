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
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <QtEndian>
#include <QMutex>

#include "musicfile_ogg.h"

struct _MusicFile_OggCore
{
    static size_t oggReadCallback(void* ptr, size_t size, size_t nmemb, void* datasource)
    { return static_cast<_MusicFile_OggCore*>(datasource)->_read(ptr, size, nmemb); }
    static int oggSeekCallback(void* datasource, ogg_int64_t offset, int whence)
    { return static_cast<_MusicFile_OggCore*>(datasource)->_seek(offset, whence); }
    static long oggTellCallback(void* datasource)
    { return static_cast<_MusicFile_OggCore*>(datasource)->_tell(); }
    static const ov_callbacks oggCallbacks;

    _MusicFile_OggCore(MusicFile_Ogg* shell_) : shell(shell_) {}

    bool test();
    bool open();
    void close();
    qint64 size();
    bool seek(qint64 pos);
    qint64 pos();
    qint64 readData(char* data, qint64 maxSize);
    qint64 writeData(const char* data, qint64 maxSize);
    size_t _read(void* ptr, size_t size, size_t nmemb);
    int _seek(qint64 offset, int whence);
    int _close();
    long _tell();

    MusicFile_Ogg *shell;
    OggVorbis_File file;
    mutable QMutex mutex;
};
const ov_callbacks _MusicFile_OggCore::oggCallbacks = {
    oggReadCallback,
    oggSeekCallback,
    NULL,
    oggTellCallback,
};

bool _MusicFile_OggCore::test()
{
    //qDebug() << Q_FUNC_INFO;
    shell->_seek(0);
    mutex.lock();
    int result = ov_test_callbacks(this, &file, NULL, 0, oggCallbacks);
    mutex.unlock();
    switch (result)
    {
        case 0:
            break;
        case OV_EREAD:
            //qDebug() << Q_FUNC_INFO << "OV_EREAD";
            return false;
        case OV_ENOTVORBIS:
            //qDebug() << Q_FUNC_INFO << "OV_ENOTVORBIS";
            return false;
        case OV_EVERSION:
            //qDebug() << Q_FUNC_INFO << "OV_EVERSION";
            return false;
        case OV_EBADHEADER:
            //qDebug() << Q_FUNC_INFO << "OV_EBADHEADER";
            return false;
        case OV_EFAULT:
            //qDebug() << Q_FUNC_INFO << "OV_EFAULT";
            return false;
        default:
            //qDebug() << Q_FUNC_INFO << "OV_UNKNOW_ERROR";
            return false;
    }
    mutex.lock();
    ov_clear(&file);
    mutex.unlock();
    return true;
}

bool _MusicFile_OggCore::open()
{
    //qDebug() << Q_FUNC_INFO;
    shell->_seek(0);
    mutex.lock();
    int result = ov_open_callbacks(this, &file, NULL, 0, oggCallbacks);
    mutex.unlock();
    switch (result)
    {
        case 0:
            break;
        case OV_EREAD:
            //qDebug() << Q_FUNC_INFO << "OV_EREAD";
            return false;
        case OV_ENOTVORBIS:
            //qDebug() << Q_FUNC_INFO << "OV_ENOTVORBIS";
            return false;
        case OV_EVERSION:
            //qDebug() << Q_FUNC_INFO << "OV_EVERSION";
            return false;
        case OV_EBADHEADER:
            //qDebug() << Q_FUNC_INFO << "OV_EBADHEADER";
            return false;
        case OV_EFAULT:
            //qDebug() << Q_FUNC_INFO << "OV_EFAULT";
            return false;
        default:
            //qDebug() << Q_FUNC_INFO << "OV_UNKNOW_ERROR";
            return false;
    }
    vorbis_info* info = ov_info(&file, -1);
    Q_ASSERT(info != NULL);
    shell->_channels = info->channels;
    shell->_samplerate = info->rate;
    shell->_bytewidth = 2;
    shell->_blockwidth = shell->_channels * 2;
    return true;
}

void _MusicFile_OggCore::close()
{
    //qDebug() << Q_FUNC_INFO;
    mutex.lock();
    ov_clear(&file);
    mutex.unlock();
}

qint64 _MusicFile_OggCore::size()
{
    mutex.lock();
    qint64 result = ov_pcm_total(&file, -1);
    mutex.unlock();
    //qDebug() << Q_FUNC_INFO << result;
    return result;
}

bool _MusicFile_OggCore::seek(qint64 pos)
{
    //qDebug() << Q_FUNC_INFO << "pos" << pos << "size" << size();
    mutex.lock();
    int result = ov_pcm_seek(&file, pos / shell->_blockwidth);
    mutex.unlock();
    switch (result)
    {
        case 0:
            break;
        case OV_ENOSEEK:
            //qDebug() << Q_FUNC_INFO << "OV_ENOSEEK";
            return false;
        case OV_EINVAL:
            //qDebug() << Q_FUNC_INFO << "OV_EINVAL";
            return false;
        case OV_EREAD:
            //qDebug() << Q_FUNC_INFO << "OV_EREAD";
            return false;
        case OV_EFAULT:
            //qDebug() << Q_FUNC_INFO << "OV_EFAULT";
            return false;
        case OV_EBADLINK:
            //qDebug() << Q_FUNC_INFO << "OV_EBADLINK";
            return false;
    }
    /*int currpos = this->pos();
    if (currpos < pos)
    {
        char *buf = new char[pos - currpos];
        this->readData(buf, pos - currpos);
        delete [] buf;
    }*/
    //qDebug() << Q_FUNC_INFO << "result" << this->pos() * shell->_blockwidth;
    return true;
}

qint64 _MusicFile_OggCore::pos()
{
    //qDebug() << Q_FUNC_INFO;
    mutex.lock();
    qint64 result = ov_pcm_tell(&file);
    mutex.unlock();
    switch (result)
    {
        case OV_EINVAL:
            //qDebug() << Q_FUNC_INFO << "OV_EINVAL";
            return false;
        default:
            break;
    }
    return result;
}

qint64 _MusicFile_OggCore::readData(char* data, qint64 maxSize)
{
    //qDebug() << Q_FUNC_INFO << maxSize;
    qint64 size = 0;
    while (size < maxSize)
    {
        int current_section;
        mutex.lock();
        qint64 result = ov_read(&file, data + size, maxSize - size, 0, 2, 1, &current_section);
        mutex.unlock();
        switch (result)
        {
            case OV_HOLE:
                //qDebug() << Q_FUNC_INFO << "OV_HOLE";
                return -1;
            case OV_EBADLINK:
                //qDebug() << Q_FUNC_INFO << "OV_EBADLINK";
                return -1;
            case OV_EINVAL:
                //qDebug() << Q_FUNC_INFO << "OV_EINVAL";
                return -1;
            default:
                break;
        }
        size += result;
    }
    //qDebug() << Q_FUNC_INFO << "result" << result;
    return size;
}

qint64 _MusicFile_OggCore::writeData(const char* /*data*/, qint64 /*maxSize*/)
{
    return -1;
}

size_t _MusicFile_OggCore::_read(void* ptr, size_t size, size_t nmemb)
{
    Q_ASSERT(shell != NULL);
    //qDebug() << Q_FUNC_INFO << size << nmemb;
    /*qint64 pos = shell->_pos();*/
    qint64 result = shell->_readData(static_cast<char*>(ptr), size * nmemb);
    //qDebug() << Q_FUNC_INFO << result;
    /* let QIODevice know the change */
    /*if (result > 0)
        shell->_seek(pos + result);*/
    return result;
}

int _MusicFile_OggCore::_seek(qint64 offset, int whence)
{
    Q_ASSERT(shell != NULL);
    //qDebug() << Q_FUNC_INFO << offset << whence;
    switch (whence)
    {
        case SEEK_SET:
            shell->_seek(offset);
            break;
        case SEEK_CUR:
            shell->_seek(shell->_pos() + offset);
            break;
        case SEEK_END:
            shell->_seek(shell->_size() + offset);
            break;
    }
    return 0;
}

long _MusicFile_OggCore::_tell()
{
    Q_ASSERT(shell != NULL);
    //qDebug() << Q_FUNC_INFO;
    return shell->_pos();
}



MusicFile_Ogg::MusicFile_Ogg(const QString& fileName) :
    MusicFile(fileName),
    _core(new _MusicFile_OggCore(this))
{
    //qDebug() << Q_FUNC_INFO << fileName;
}

MusicFile_Ogg::MusicFile_Ogg(const MusicData& fileDescription) :
    MusicFile(fileDescription),
    _core(new _MusicFile_OggCore(this))
{
    //qDebug() << Q_FUNC_INFO << _fileName;
}

MusicFile_Ogg::~MusicFile_Ogg()
{
    //qDebug() << Q_FUNC_INFO;
    close();
    delete _core;
}

bool MusicFile_Ogg::open(OpenMode mode)
{
    //qDebug() << Q_FUNC_INFO << mode;
    if (!MusicFile::open(mode))
        return false;
    MusicFile::seek(0);
    if (!_core->test())
        goto err;
    if (!_core->open())
        goto err;
    if (_loopEnd == 3724704)
    {
        //qDebug() << fileName();
    }
    return true;
err:
    MusicFile::close();
    return false;
}

void MusicFile_Ogg::close()
{
    //qDebug() << Q_FUNC_INFO;
    if (openMode() != NotOpen)
    {
        _core->close();
    }
    MusicFile::close();
}

qint64 MusicFile_Ogg::pos() const
{
    //qDebug() << Q_FUNC_INFO;
    return _core->pos();
}

qint64 MusicFile_Ogg::size() const
{
    //qDebug() << Q_FUNC_INFO;
    return _core->size();
}

bool MusicFile_Ogg::seek(qint64 pos)
{
    //qDebug() << Q_FUNC_INFO;
    return _core->seek(pos);
}

bool MusicFile_Ogg::reset()
{
    //qDebug() << Q_FUNC_INFO;
    return MusicFile::reset() && _core->seek(0);
}

qint64 MusicFile_Ogg::readData(char* data, qint64 maxSize)
{
    //qDebug() << Q_FUNC_INFO;
    return _core->readData(data, maxSize);
}

qint64 MusicFile_Ogg::writeData(const char* data, qint64 maxSize)
{
    //qDebug() << Q_FUNC_INFO;
    return _core->writeData(data, maxSize);
}
