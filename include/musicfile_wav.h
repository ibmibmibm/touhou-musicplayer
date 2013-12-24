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
#include "musicfile.h"

class MusicFile_Wav : public MusicFile
{
    Q_OBJECT

    protected:
        MusicFile_Wav(const MusicData& fileDescription);

    public:
        typedef MusicFile::OpenMode OpenMode;
        friend class MusicFileFactory;
        virtual ~MusicFile_Wav() {}

        virtual bool open(OpenMode mode);
        virtual qint64 size() const;
        virtual bool seek(qint64 pos);
        virtual bool reset();
        enum _Endian
        {
            _LittleEndian,
            _BigEndian,
        };

    protected:
        virtual qint64 readData(char * data, qint64 maxSize);

    private:
        bool _parseHeader();
        template <typename T, typename U>
        bool _getFieldFromFile(U &data);
        void _removeLeadingZeros();
        void _initializeAsRawData();
        _Endian _endian;
        uint _format;
        uint _bytespersec;
        uint _blockalign;

        qint64 _dataBegin;
        qint64 _dataSize;
        qint64 _dataEnd;

    public:
        static MusicFile* createFunction(const MusicData& fileDescription) { return new MusicFile_Wav(fileDescription); }
};

#endif // MUSICFILE_WAV_H
