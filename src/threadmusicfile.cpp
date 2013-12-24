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
#include <QtDebug>
#include "threadmusicfile.h"

const size_t _bufferSize = 1024;

ThreadMusicFile::ThreadMusicFile(const MusicData& musicData, uint totalLoop) :
    _musicFile(new LoopMusicFile(musicData, totalLoop)),
    _fileBuffer(NULL),
    _bufferSampleSize(1024*500),
    _stoped(false)
{
    //qDebug() << Q_FUNC_INFO;
}

bool ThreadMusicFile::open(MusicFile::OpenMode mode)
{
    //qDebug() << Q_FUNC_INFO;
    if (_musicFile == NULL)
        return false;
    if (_musicFile->open(mode))
    {
        _samplePos = 0,
        _musicFile->sampleSeek(0);
        _fileBuffer = new char[blockwidth() * _bufferSize];
        start();
        return true;
    }
    return false;
}

void ThreadMusicFile::close()
{
    //qDebug() << Q_FUNC_INFO << "begin";
    _stoped = true;
    //qDebug() << Q_FUNC_INFO << "wait";
    wait();
    if (_fileBuffer)
    {
        delete _fileBuffer;
        _fileBuffer = NULL;
    }
    //qDebug() << Q_FUNC_INFO << "end";
}

void ThreadMusicFile::run()
{
    //qDebug() << Q_FUNC_INFO;
    _stoped = false;
    while (!_stoped)
    {
        _bufferMutex.lock();
        while (_buffer.size() >= _bufferSampleSize)
        {
            while (!_bufferNotFull.wait(&_bufferMutex, 100))
                if (_stoped)
                    return;
        }
        _bufferMutex.unlock();
        _fillBuffer();
    }
}

bool ThreadMusicFile::_fillBuffer()
{
    //qDebug() << Q_FUNC_INFO;
    _bufferMutex.lock();
    qint64 sample = _musicFile->samplePos() - _buffer.size() / blockwidth();
    if (sample != _samplePos)
    {
        _musicFile->sampleSeek(_samplePos);
        _buffer.clear();
    }
    qint64 size = _musicFile->sampleRead(_fileBuffer, _bufferSize);
    if (size <= 0)
    {
        _bufferMutex.unlock();
        return false;
    }
    sample = size * blockwidth();
    for (uint i = 0; i < sample; ++i)
        _buffer.enqueue(_fileBuffer[i]);
    //_bufferNotEmpty.wakeAll();
    _bufferMutex.unlock();
    return true;
}

qint64 ThreadMusicFile::sampleRead(char* buffer, qint64 needSample)
{
    //qDebug() << Q_FUNC_INFO << needSample << _samplePos;
    needSample = qMin<qint64>(needSample, sampleSize() - _samplePos);
    int size = needSample * blockwidth();
    forever
    {
        _bufferMutex.lock();
        if (_buffer.size() < size)
        {
            _bufferMutex.unlock();
            if (!_fillBuffer())
            {
                _bufferMutex.lock();
                size = _buffer.size();
                _bufferMutex.unlock();
            }
        }
        else
        {
            for (int i = 0; i < size; ++i)
                buffer[i] = _buffer.dequeue();
            _samplePos += needSample;
            _bufferMutex.unlock();
            _bufferNotFull.wakeAll();
            return needSample;
        }
    }
}

bool ThreadMusicFile::sampleSeek(qint64 samples)
{
    //qDebug() << Q_FUNC_INFO << samples;
    _samplePos = samples;
    return true;
}
