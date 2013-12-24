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
#ifndef MUSICFILE_OGG_H
#define MUSICFILE_OGG_H
#include "musicfile.h"

struct _MusicFile_OggCore;
class MusicFile_Ogg : public MusicFile
{
    Q_OBJECT
    public:
        typedef MusicFile::OpenMode OpenMode;

        MusicFile_Ogg(const QString& fileName);
        MusicFile_Ogg(const MusicData& fileDescription);
        virtual ~MusicFile_Ogg();

        virtual bool open(OpenMode mode);
        virtual void close();
        virtual qint64 pos() const;
        virtual qint64 size() const;
        virtual bool seek(qint64 pos);
        virtual bool reset();

    protected:
        virtual qint64 readData(char* data, qint64 maxSize);
        virtual qint64 writeData(const char* data, qint64 maxSize);

        friend struct _MusicFile_OggCore;
        _MusicFile_OggCore* _core;
};

#endif // MUSICFILE_OGG_H
