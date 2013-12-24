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
#ifndef THREADMUSICFILE_H
#define THREADMUSICFILE_H
#include <QWaitCondition>
#include <QMutexLocker>
#include <QThread>
#include <QQueue>
#include "loopmusicfile.h"

class ThreadMusicFile : public QThread
{
    Q_OBJECT
    private:
        ThreadMusicFile(const ThreadMusicFile&);
        ThreadMusicFile&operator=(const ThreadMusicFile&);
    public:
        ThreadMusicFile(const MusicData& musicData, uint totalLoop);
        ~ThreadMusicFile() { close(); delete _musicFile; }

        bool open(MusicFile::OpenMode mode);
        void close();
        QString errorString() const { Q_ASSERT(_musicFile != NULL); return _musicFile->errorString(); }

        qint64 samplePos() const { return _samplePos; }
        qint64 sampleSize() const { Q_ASSERT(_musicFile != NULL); return _musicFile->sampleSize(); }
        bool sampleSeek(qint64 pos);
        qint64 sampleRead(char* buffer, qint64 maxSample);

        uint channels() const { Q_ASSERT(_musicFile != NULL); return _musicFile->channels(); }
        uint samplerate() const { Q_ASSERT(_musicFile != NULL); return _musicFile->samplerate(); }
        uint bytewidth() const { Q_ASSERT(_musicFile != NULL); return _musicFile->bytewidth(); }
        uint blockwidth() const { Q_ASSERT(_musicFile != NULL); return _musicFile->blockwidth(); }
        uint loop() const { Q_ASSERT(_musicFile != NULL); return _musicFile->loop(); }
        uint totalLoop() const { Q_ASSERT(_musicFile != NULL); return _musicFile->totalLoop(); }

        int bufferSize() const { QMutexLocker locker(&_bufferMutex); return _buffer.size(); }
    protected:
        virtual void run();
    private:
        bool _fillBuffer();
        LoopMusicFile* _musicFile;
        QQueue<char> _buffer;
        char *_fileBuffer;
        qint64 _bufferSampleSize;
        mutable QMutex _bufferMutex;
        QWaitCondition _bufferNotFull;
        qint64 _samplePos;
        qint64 _buuferSamplePos;
        volatile bool _stoped;
};

#endif // THREADMUSICFILE_H
