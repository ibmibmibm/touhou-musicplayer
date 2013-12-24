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
#ifndef LOOPMUSICFILE_H
#define LOOPMUSICFILE_H
#include "musicfile.h"

class LoopMusicFile: public QObject
{
    Q_OBJECT

    private:
        LoopMusicFile(const LoopMusicFile&);
        LoopMusicFile&operator=(const LoopMusicFile&);

    public:
        LoopMusicFile(const MusicData& musicData, uint totalLoop = 1);
        ~LoopMusicFile() { delete _musicFile; }

        bool open(MusicFile::OpenMode mode);
        QString errorString() const { return _errorString; }

        qint64 samplePos() const { return _samples; };
        qint64 sampleSize() const { return _totalSamples; };
        bool sampleSeek(qint64 pos);
        qint64 sampleRead(char* buffer, qint64 maxSample);
        QByteArray sampleRead(qint64 maxSample);

        uint channels() const { Q_ASSERT(_musicFile != NULL); return _musicFile->channels(); }
        uint samplerate() const { Q_ASSERT(_musicFile != NULL); return _musicFile->samplerate(); }
        uint bytewidth() const { Q_ASSERT(_musicFile != NULL); return _musicFile->bytewidth(); }
        uint blockwidth() const { Q_ASSERT(_musicFile != NULL); return _musicFile->blockwidth(); }
        uint loop() const { return _loop; }
        uint totalLoop() const { return _totalLoop; }

    protected:
        void setErrorString(QString newErrorString) { _errorString = newErrorString; }

    private:
        uint _samplesToLoop(qint64& samples);
        void _setLoop(uint newLoop);
        void _setSamplesAndLoop(qint64 newSamples);
        qint64 _samples;
        qint64 _totalSamples;
        uint _loop;
        uint _totalLoop;
        uint _fadeoutTime;
        qint64 _fadeoutSamples;
        MusicFile* _musicFile;
        QString _errorString;
};

#endif // LOOPMUSICFILE_H
