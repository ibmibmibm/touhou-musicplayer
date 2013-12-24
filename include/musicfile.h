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
#ifndef MUSICFILE_H
#define MUSICFILE_H
#include <QIODevice>
#include <QString>
#include <QHash>
#include "musicdata.h"

class QAbstractFileEngine;
class MusicFileFactory;

class MusicFile : public QIODevice
{
    Q_OBJECT

    protected:
        MusicFile();
        MusicFile(const MusicData& fileDescription);

/* outer layer */
    public:
        typedef QIODevice::OpenMode OpenMode;

        virtual ~MusicFile() {}

        const QString& fileName() const;
        void setFileName(const QString& fileName);
        void setFileDescription(const MusicData& fileDescription);

        qint64 loopBegin() const { return _loopBegin; }
        void setLoopStart(qint64 newLoopStart) { _loopBegin = newLoopStart; }
        qint64 loopEnd() const { return _loopEnd; }
        void setLoopEnd(qint64 newLoopEnd) { _loopEnd = newLoopEnd; }
        QString title() const { return _title; }
        void setTitle(QString newTitle) { _title = newTitle; }
        QString album() const { return _album; }
        void setAlbum(QString newAlbum) { _album = newAlbum; }

        virtual bool open(OpenMode mode);
        virtual qint64 size() const;
        virtual bool seek(qint64 pos);
        virtual bool reset();
        virtual qint64 bytesAvailable() const { return size(); }

        virtual qint64 samplePos() const { return pos() / _blockwidth; }
        virtual qint64 sampleSize() const { return size() / _blockwidth; }
        virtual bool sampleSeek(qint64 pos) { return seek(pos * _blockwidth); }
        virtual qint64 sampleRead(char* buffer, qint64 maxSize) { return read(buffer, maxSize * _blockwidth) / _blockwidth; }

        uint channels() const { return _channels; }
        uint samplerate() const { return _samplerate; }
        uint bytewidth() const { return _bytewidth; }
        uint blockwidth() const { return _blockwidth; }


/* middle layer */
    protected:
        virtual qint64 readData(char* data, qint64 maxSize);
        virtual qint64 writeData(const char*, qint64) { Q_ASSERT(false); return -1; }
        void setErrorString(const QString & str);
        void setOpenMode(OpenMode openMode);

        uint _channels;
        uint _samplerate;
        uint _bytewidth;
        uint _blockwidth;

        bool _loop;
        qint64 _loopBegin;
        qint64 _loopEnd;
        QString _title;
        QString _album;
        QExplicitlySharedDataPointer<ArchiveMusicData> _archiveMusicData;


/* inner layer */
        qint64 _pos() const;
        qint64 _size() const;
        bool _seek(qint64 pos);
        bool _reset();
        qint64 _readData(char* data, qint64 maxSize);
        qint64 _writeData(const char* data, qint64 maxSize);

    private:
        QString _fileName;
        QAbstractFileEngine* _fileEngine;
        MusicFile(const MusicFile&);
        MusicFile& operator=(const MusicFile&);
};

class MusicFileFactory
{
    public:
        typedef MusicFile* (*CreateFunction)(const MusicData&);
        static int registerMusicFile(const QString& suffix, CreateFunction createFunction);
        static MusicFile* createMusicFile(const MusicData& fileDescription);
    private:
        static QHash<QString, CreateFunction> functionHash;
};

#endif // MUSICFILE_H
