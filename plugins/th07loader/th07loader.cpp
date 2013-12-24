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
#include <QDir>
#include <QFile>
#include <QList>
#include <QFileInfo>

#include "th07loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th07Loader", Th07Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Perfect Cherry Blossom.");
    const QString SongData[] = {
        QString::fromWCharArray(L"\u5996\u3005\u5922\u3000\u301c Snow or Cherry Petal"),
        QString::fromWCharArray(L"\u7121\u4f55\u6709\u306e\u90f7\u3000\u301c Deep Mountain"),
        QString::fromWCharArray(L"\u30af\u30ea\u30b9\u30bf\u30e9\u30a4\u30ba\u30b7\u30eb\u30d0\u30fc"),
        QString::fromWCharArray(L"\u9060\u91ce\u5e7b\u60f3\u7269\u8a9e"),
        QString::fromWCharArray(L"\u30c6\u30a3\u30a2\u30aa\u30a4\u30a8\u30c4\u30a9\u30f3(withered leaf)"),
        QString::fromWCharArray(L"\u30d6\u30af\u30ec\u30b7\u30e5\u30c6\u30a3\u306e\u4eba\u5f62\u5e2b"),
        QString::fromWCharArray(L"\u4eba\u5f62\u88c1\u5224\u3000\u301c \u4eba\u306e\u5f62\u5f04\u3073\u3057\u5c11\u5973"),
        QString::fromWCharArray(L"\u5929\u7a7a\u306e\u82b1\u306e\u90fd"),
        QString::fromWCharArray(L"\u5e7d\u970a\u697d\u56e3\u3000\u301c Phantom Ensemble"),
        QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Ancient Temple"),
        QString::fromWCharArray(L"\u5e83\u6709\u5c04\u602a\u9ce5\u4e8b\u3000\u301c Till When?"),
        QString::fromWCharArray(L"\u30a2\u30eb\u30c6\u30a3\u30e1\u30c3\u30c8\u30c8\u30a5\u30eb\u30fc\u30b9"),
        QString::fromWCharArray(L"\u5e7d\u96c5\u306b\u54b2\u304b\u305b\u3001\u58a8\u67d3\u306e\u685c\u3000\u301c Border of Life"),
        QString::fromWCharArray(L"\u30dc\u30fc\u30c0\u30fc\u30aa\u30d6\u30e9\u30a4\u30d5"),
        QString::fromWCharArray(L"\u5996\u3005\u8dcb\u6248"),
        QString::fromWCharArray(L"\u5c11\u5973\u5e7b\u846c\u3000\u301c Necro-Fantasy"),
        QString::fromWCharArray(L"\u5996\u3005\u8dcb\u6248\u3000\u301c Who done it!"),
        QString::fromWCharArray(L"\u30cd\u30af\u30ed\u30d5\u30a1\u30f3\u30bf\u30b8\u30a2"),
        QString::fromWCharArray(L"\u6625\u98a8\u306e\u5922"),
        QString::fromWCharArray(L"\u3055\u304f\u3089\u3055\u304f\u3089\u3000\u301c Japanize Dream..."),
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("Th07.dat");
    const QString BgmName("Thbgm.dat");
}

const QString& Th07Loader::title() const
{
    return Title;
}

uint Th07Loader::size() const
{
    return SongDataSize;
}

bool Th07Loader::open(const QString &path)
{
    dir = QDir(path);
    if (!dir.exists(FileName) || !dir.exists(BgmName))
        return false;

// PBG4 decompresser
    QFile file(dir.filePath(FileName));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    quint32 max_file_count;
    quint32 header_pos;
    quint32 header_dsize;
    quint32 header_csize;
    {
        quint32 magicNumber;
        if (file.read(reinterpret_cast<char*>(&magicNumber), 4) != 4)
            return false;
        if (qFromLittleEndian(magicNumber) != 0x34474250) //PBG4
            return false;
// Stage 1
        if (file.read(reinterpret_cast<char*>(&max_file_count), 4) != 4)
            return false;
        max_file_count = qFromLittleEndian(max_file_count);

        if (file.read(reinterpret_cast<char*>(&header_pos), 4) != 4)
            return false;
        header_pos = qFromLittleEndian(header_pos);

        if (file.read(reinterpret_cast<char*>(&header_dsize), 4) != 4)
            return false;
        header_dsize = qFromLittleEndian(header_dsize);

        if (file.size() <= header_pos)
            return false;
        header_csize = file.size() - header_pos;
    }

    QByteArray thbgm_data;
    {
// Stage 2
        file.seek(header_pos);
        QByteArray header = lzDecompress(file.read(header_csize));
        Q_ASSERT(header.size() == header_dsize);
        uint thbgm_offset;
        uint thbgm_dsize;
        uint thbgm_csize;
        char* cursor = header.data();
        for (uint i = 0; ; ++i)
        {
            if (i == max_file_count)
                return false;
            QString name(cursor);
            cursor += name.size() + 1;
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }
            thbgm_offset = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)); //offset
            cursor += 4;
            thbgm_dsize = qFromLittleEndian<qint64>(reinterpret_cast<uchar*>(cursor)); //size
            cursor += 8;
            while (*cursor++)
                ;
            thbgm_csize = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)) - thbgm_offset;
            break;
        }
        file.seek(thbgm_offset);
        thbgm_data = lzDecompress(file.read(thbgm_csize), thbgm_dsize);
        Q_ASSERT(thbgm_data.size() == thbgm_dsize);
    }
// Stage3
    {
        ThbgmData* thbgmData = reinterpret_cast<ThbgmData*>(thbgm_data.data());
        for (uint i = 0; i < SongDataSize; ++i)
        {
            FileInfo info;
            info.name = thbgmData[i].name;
            info.offset = qFromLittleEndian<qint32>(thbgmData[i].offset);
            info.checksum = qFromLittleEndian<qint32>(thbgmData[i].checksum);
            info.loopBegin = qFromLittleEndian<qint32>(thbgmData[i].loopBegin) >> 2;
            info.loopEnd = qFromLittleEndian<qint32>(thbgmData[i].loopEnd) >> 2;
            info.header = QByteArray(thbgmData[i].header, 16);
            if (i)
                info_list[i - 1].size = info.offset - info_list[i - 1].offset;
            info_list << info;
        }
        info_list[SongDataSize - 1].size = QFileInfo(dir.filePath(BgmName)).size() - info_list[SongDataSize - 1].offset;
    }

    QFile wav(dir.filePath(BgmName));
    if (!wav.open(QIODevice::ReadOnly))
        return false;
    return true;
}

MusicData Th07Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list.at(index);
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(BgmName), info.offset, info.offset + info.size);

    return MusicData(
        info.name,
        SongData[index],
        Title,
        ".wav",
        info.size,
        true,
        info.loopBegin,
        info.loopEnd,
        &archiveMusicData
    );
}

QByteArray Th07Loader::content(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list.at(index);
    QFile file(dir.filePath(BgmName));
    file.open(QIODevice::ReadOnly);
    file.seek(info.offset);
    int x;
    QByteArray wav("RIFF");
    QByteArray xdata(4, '\0');
    x = info.size + 36;
    setUInt32(x, xdata.data());
    wav.append(xdata);
    wav.append("WAVEfmt ");
    x = 16;
    setUInt32(x, xdata.data());
    wav.append(xdata);
    wav.append(info.header);
    wav.append("data");
    x = info.size;
    setUInt32(x, xdata.data());
    wav.append(xdata);
    wav.append(file.read(info.size));

    return wav;
}

void Th07Loader::close()
{
    info_list.clear();
}
