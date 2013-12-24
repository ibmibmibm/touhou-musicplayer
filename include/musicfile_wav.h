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
#ifndef MUSICFILE_WAV_H
#define MUSICFILE_WAV_H
#include <QFile>
#include <QString>
#include "musicfile.h"

class MusicFile_Wav : public MusicFile
{
    Q_OBJECT
    public:
        typedef MusicFile::OpenMode OpenMode;

        MusicFile_Wav(const QString& fileName);
        MusicFile_Wav(const MusicData& fileDescription);
        virtual ~MusicFile_Wav() {}

        virtual bool open(OpenMode mode);
        virtual qint64 pos() const;
        virtual qint64 size() const;
        virtual bool seek(qint64 pos);
        virtual bool reset();
        enum _Endian
        {
            _LittleEndian,
            _BigEndian,
        };

        uint format() const { return _format; }
        uint bytespersec() const { return _bytespersec; }
        uint blockalign() const { return _blockalign; }

    protected:
        virtual qint64 readData(char * data, qint64 maxSize);
        virtual qint64 writeData(const char * data, qint64 maxSize);
        void setErrorString(const QString & str);
        void setOpenMode(OpenMode openMode);

        bool _readyReadWrite;
        _Endian _endian;
        uint _format;
        uint _bytespersec;
        uint _blockalign;

        qint64 _dataBegin;
        qint64 _dataSize;
        qint64 _dataEnd;

    private:
        bool _parseHeader();
};

#endif // MUSICFILE_WAV_H
