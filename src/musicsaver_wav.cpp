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
#include <QtEndian>
#include "musicsaver_wav.h"
#include "musicfile_ogg.h"
#include "musicfile_wav.h"

bool MusicSaver_Wav::save(const QString& filename, MusicData musicData, uint loop, uint fadeoutTime)
{
    MusicFile* musicFile;
    if (musicData.suffix() == ".ogg")
    {
        musicFile = new MusicFile_Ogg(musicData);
        if (!musicFile->open(QIODevice::ReadOnly))
        {
            setErrorString(musicFile->errorString());
            delete musicFile;
            return false;
        }
    }
    else if (musicData.suffix() == ".wav")
    {
        musicFile = new MusicFile_Wav(musicData);
        if (!musicFile->open(QIODevice::ReadOnly))
        {
            setErrorString(musicFile->errorString());
            delete musicFile;
            return false;
        }
    }
    else
        return false;
    qint64 fadeoutSample = (static_cast<qint64>(fadeoutTime) * musicFile->samplerate()) * 0.001;
    if (fadeoutSample > (musicFile->loopEnd() - musicFile->loopBegin()))
    {
        setErrorString(QObject::tr("Fadeout time is too long."));
        delete musicFile;
        return false;
    }
    quint64 fadeoutSize = fadeoutSample * musicFile->blockwidth();
    quint64 size = (musicFile->loopBegin() + (musicFile->loopEnd() - musicFile->loopBegin()) * loop) * musicFile->blockwidth() + fadeoutSize;
    if (size > Q_UINT64_C(4294967295))
    {
        setErrorString(QObject::tr("Repeat value is too large."));
        delete musicFile;
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        setErrorString(file.errorString());
        delete musicFile;
        return false;
    }

    quint32 int32;
    quint16 int16;
    file.write("RIFF");
    int32 = qToLittleEndian<qint32>(size + 36u);
    file.write(reinterpret_cast<const char*>(&int32), 4);
    file.write("WAVEfmt ");
    int32 = qToLittleEndian<qint32>(16u);
    file.write(reinterpret_cast<const char*>(&int32), 4);

    // format
    int16 = qToLittleEndian<qint16>(1u); // WAVE_FORMAT_PCM
    file.write(reinterpret_cast<const char*>(&int16), 2);
    // channels
    int16 = qToLittleEndian<qint16>(musicFile->channels());
    file.write(reinterpret_cast<const char*>(&int16), 2);
    // samplerate
    int32 = qToLittleEndian<qint32>(musicFile->samplerate());
    file.write(reinterpret_cast<const char*>(&int32), 4);
    // bytespersec
    int32 = qToLittleEndian<qint32>(musicFile->samplerate() * musicFile->blockwidth());
    file.write(reinterpret_cast<const char*>(&int32), 4);
    // blockalign
    int16 = qToLittleEndian<qint32>(4);
    file.write(reinterpret_cast<const char*>(&int16), 2);
    // bitwidth
    int16 = qToLittleEndian<qint32>(musicFile->bytewidth() << 3);
    file.write(reinterpret_cast<const char*>(&int16), 2);

    file.write("data");
    int32 = qToLittleEndian<qint32>(size);
    file.write(reinterpret_cast<const char*>(&int32), 4);

    musicFile->sampleSeek(0);
    file.write(musicFile->read(musicFile->loopBegin() * musicFile->blockwidth()));
    for (uint i = 0; i < loop; ++i)
    {
        file.write(musicFile->read((musicFile->loopEnd() - musicFile->loopBegin()) * musicFile->blockwidth()));
        musicFile->sampleSeek(musicFile->loopBegin());
    }
    QByteArray fadeoutData = musicFile->read(fadeoutSize);

    qint16 *out = reinterpret_cast<qint16*>(fadeoutData.data());
    for (qint64 i = 0; i < fadeoutSample; ++i)
    {
        qreal volume = fadeoutVolume(fadeoutSample, i);
        for (uint j = 0; j < musicFile->channels(); ++j)
        {
            *out *= volume;
            ++out;
        }
    }
    musicFile->close();
    delete musicFile;
    musicFile = NULL;
    file.write(fadeoutData);
    file.close();
    return true;
}
