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
#include <QtDebug>

#include "th07loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th07Loader", Th07Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Perfect Cherry Blossom.");
    const QString SongData[][2] = {
        {"01", QString::fromWCharArray(L"\u5996\u3005\u5922\u3000\u301c Snow or Cherry Petal")},
        {"02", QString::fromWCharArray(L"\u7121\u4f55\u6709\u306e\u90f7\u3000\u301c Deep Mountain")},
        {"03", QString::fromWCharArray(L"\u30af\u30ea\u30b9\u30bf\u30e9\u30a4\u30ba\u30b7\u30eb\u30d0\u30fc")},
        {"04", QString::fromWCharArray(L"\u9060\u91ce\u5e7b\u60f3\u7269\u8a9e")},
        {"05", QString::fromWCharArray(L"\u30c6\u30a3\u30a2\u30aa\u30a4\u30a8\u30c4\u30a9\u30f3(withered leaf)")},
        {"06", QString::fromWCharArray(L"\u30d6\u30af\u30ec\u30b7\u30e5\u30c6\u30a3\u306e\u4eba\u5f62\u5e2b")},
        {"07", QString::fromWCharArray(L"\u4eba\u5f62\u88c1\u5224\u3000\u301c \u4eba\u306e\u5f62\u5f04\u3073\u3057\u5c11\u5973")},
        {"08", QString::fromWCharArray(L"\u5929\u7a7a\u306e\u82b1\u306e\u90fd")},
        {"09", QString::fromWCharArray(L"\u5e7d\u970a\u697d\u56e3\u3000\u301c Phantom Ensemble")},
        {"10", QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Ancient Temple")},
        {"11", QString::fromWCharArray(L"\u5e83\u6709\u5c04\u602a\u9ce5\u4e8b\u3000\u301c Till When?")},
        {"12", QString::fromWCharArray(L"\u30a2\u30eb\u30c6\u30a3\u30e1\u30c3\u30c8\u30c8\u30a5\u30eb\u30fc\u30b9")},
        {"13", QString::fromWCharArray(L"\u5e7d\u96c5\u306b\u54b2\u304b\u305b\u3001\u58a8\u67d3\u306e\u685c\u3000\u301c Border of Life")},
        {"13b", QString::fromWCharArray(L"\u30dc\u30fc\u30c0\u30fc\u30aa\u30d6\u30e9\u30a4\u30d5")},
        {"16", QString::fromWCharArray(L"\u5996\u3005\u8dcb\u6248")},
        {"17", QString::fromWCharArray(L"\u5c11\u5973\u5e7b\u846c\u3000\u301c Necro-Fantasy")},
        {"18", QString::fromWCharArray(L"\u5996\u3005\u8dcb\u6248\u3000\u301c Who done it!")},
        {"19", QString::fromWCharArray(L"\u30cd\u30af\u30ed\u30d5\u30a1\u30f3\u30bf\u30b8\u30a2")},
        {"14", QString::fromWCharArray(L"\u6625\u98a8\u306e\u5922")},
        {"15", QString::fromWCharArray(L"\u3055\u304f\u3089\u3055\u304f\u3089\u3000\u301c Japanize Dream...")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("Th07.dat");
    const QString BgmName("Thbgm.dat");
    const QString WavName("th07_%1.wav");

    bool checkAllFileExists(const QDir& programDirectory)
    {
        if (!programDirectory.exists(FileName) || !programDirectory.exists(BgmName))
            return false;
        QFile wav(programDirectory.filePath(BgmName));
        if (!wav.open(QIODevice::ReadOnly))
            return false;
        return true;
    }
}

namespace PBG4
{
    bool checkMagicNumber(QFile& file)
    {
        quint32 magicNumber;
        if (file.read(reinterpret_cast<char*>(&magicNumber), 4) != 4)
            return false;
        if (qFromLittleEndian(magicNumber) != 0x34474250) //PBG4
            return false;
        return true;
    }

    bool getHeaderDescription(QFile& file, uint& maxFileCount, uint& headerPos, uint& headerOriginalSize, uint& headerCompressedSize)
    {
        quint32 value;
        if (file.read(reinterpret_cast<char*>(&value), 4) != 4)
            return false;
        maxFileCount = qFromLittleEndian(value);

        if (file.read(reinterpret_cast<char*>(&value), 4) != 4)
            return false;
        headerPos = qFromLittleEndian(value);

        if (file.read(reinterpret_cast<char*>(&value), 4) != 4)
            return false;
        headerOriginalSize = qFromLittleEndian(value);

        if (file.size() <= headerPos)
            return false;
        headerCompressedSize = file.size() - headerPos;
        return true;
    }

    bool getThbgm(QFile& file, const uint maxFileCount, const uint headerPos, const uint headerOriginalSize, const uint headerCompressedSize, QByteArray& thbgm)
    {
        file.seek(headerPos);
        QByteArray header = lzDecompress(file.read(headerCompressedSize));
        if (header.size() != static_cast<int>(headerOriginalSize))
            return false;
        uint thbgmOffset;
        uint thbgmOriginalSize;
        uint thbgmCompressedSize;
        char* cursor = header.data();
        for (uint i = 0; ; ++i)
        {
            if (i == maxFileCount)
                return false;
            QString name(cursor);
            cursor += name.size() + 1;
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }
            thbgmOffset = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)); //offset
            cursor += 4;
            thbgmOriginalSize = qFromLittleEndian<qint64>(reinterpret_cast<uchar*>(cursor)); //size
            cursor += 8;
            while (*cursor++)
                ;
            thbgmCompressedSize = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)) - thbgmOffset;
            break;
        }
        file.seek(thbgmOffset);
        thbgm = lzDecompress(file.read(thbgmCompressedSize), thbgmOriginalSize);
        Q_ASSERT(thbgm.size() == thbgmOriginalSize);
        return true;
    }

    bool parser(const QString& filename, QByteArray& thbgm)
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            return false;
        if (!PBG4::checkMagicNumber(file))
            return false;

        uint maxFileCount;
        uint headerPos;
        uint headerOriginalSize;
        uint headerCompressedSize;
        if (!getHeaderDescription(file, maxFileCount, headerPos, headerOriginalSize, headerCompressedSize))
            return false;

        getThbgm(file, maxFileCount, headerPos, headerOriginalSize, headerCompressedSize, thbgm);
        return true;
    }
}

namespace THBGM
{
    bool parser(const QString& filename, const QByteArray& thbgm, QHash<QString, FileInfo>& infoHash)
    {
        const ThbgmData* thbgmData = reinterpret_cast<const ThbgmData*>(thbgm.data());
        QList<FileInfo> info_list;
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
        info_list[SongDataSize - 1].size = QFileInfo(filename).size() - info_list[SongDataSize - 1].offset;
        foreach(FileInfo info, info_list)
        {
            infoHash.insert(info.name, info);
        }
        return true;
    }
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
    programDirectory = QDir(path);

    if (!checkAllFileExists(programDirectory))
        return false;

    QByteArray thbgm;
    if (!PBG4::parser(programDirectory.filePath(FileName), thbgm))
        return false;

    if (!THBGM::parser(programDirectory.filePath(BgmName), thbgm, infoHash))
        return false;

    return true;
}

MusicData Th07Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = infoHash.value(WavName.arg(SongData[index][0]));
    ArchiveMusicData archiveMusicData(programDirectory.absoluteFilePath(BgmName), info.offset, info.offset + info.size);

    return MusicData(
        info.name,
        SongData[index][1],
        "ZUN",
        Title,
        index + 1,
        SongDataSize,
        ".wav",
        info.size,
        true,
        info.loopBegin,
        info.loopEnd,
        &archiveMusicData
    );
}

void Th07Loader::close()
{
    infoHash.clear();
}
