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
#include <QFile>
#include <QtEndian>
#include "musicsaver_wav.h"
#include "loopmusicfile.h"

bool MusicSaver_Wav::save(const QString& filename, MusicData musicData, uint loop)
{
    //qDebug() << Q_FUNC_INFO;

    LoopMusicFile* musicFile = new LoopMusicFile(musicData, loop);

    if (musicFile == NULL)
        return false;

    if (!musicFile->open(QIODevice::ReadOnly))
    {
        setErrorString(musicFile->errorString());
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

    quint64 totalSize = musicFile->sampleSize() * musicFile->blockwidth();
    if (totalSize > (Q_UINT64_C(4294967295) - Q_UINT64_C(36)))
    {
        setErrorString(QObject::tr("Repeat value is too large."));
        delete musicFile;
        return false;
    }

    quint32 int32;
    quint16 int16;
    file.write("RIFF");
    int32 = qToLittleEndian<qint32>(totalSize + 36u);
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
    int32 = qToLittleEndian<qint32>(totalSize);
    file.write(reinterpret_cast<const char*>(&int32), 4);

    const qint64 bufferSample = 65536;
    musicFile->sampleSeek(0);
    while (file.write(musicFile->sampleRead(bufferSample)) > 0)
        ;
    file.close();
    delete musicFile;
    musicFile = NULL;
    return true;
}
