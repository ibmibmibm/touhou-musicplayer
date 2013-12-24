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
#include <QHash>
#include <QByteArray>

#include "th06loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th06Loader", Th06Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u7d05\u9b54\u90f7\u3000\u301c the Embodiment of Scarlet Devil.");
    const QString SongData[] = {
        QString::fromWCharArray(L"\u8d64\u3088\u308a\u7d05\u3044\u5922"),
        QString::fromWCharArray(L"\u307b\u304a\u305a\u304d\u307f\u305f\u3044\u306b\u7d05\u3044\u9b42"),
        QString::fromWCharArray(L"\u5996\u9b54\u591c\u884c"),
        QString::fromWCharArray(L"\u30eb\u30fc\u30cd\u30a4\u30c8\u30a8\u30eb\u30d5"),
        QString::fromWCharArray(L"\u304a\u3066\u3093\u3070\u604b\u5a18"),
        QString::fromWCharArray(L"\u4e0a\u6d77\u7d05\u8336\u9928\u3000\u301c Chinese Tea"),
        QString::fromWCharArray(L"\u660e\u6cbb\u5341\u4e03\u5e74\u306e\u4e0a\u6d77\u30a2\u30ea\u30b9"),
        QString::fromWCharArray(L"\u30f4\u30ef\u30eb\u9b54\u6cd5\u56f3\u66f8\u9928"),
        QString::fromWCharArray(L"\u30e9\u30af\u30c8\u30ac\u30fc\u30eb\u3000\u301c \u5c11\u5973\u5bc6\u5ba4"),
        QString::fromWCharArray(L"\u30e1\u30a4\u30c9\u3068\u8840\u306e\u61d0\u4e2d\u6642\u8a08"),
        QString::fromWCharArray(L"\u6708\u6642\u8a08\u3000\u301c \u30eb\u30ca\u30fb\u30c0\u30a4\u30a2\u30eb"),
        QString::fromWCharArray(L"\u30c4\u30a7\u30da\u30b7\u30e5\u306e\u5e7c\u304d\u672b\u88d4"),
        QString::fromWCharArray(L"\u4ea1\u304d\u738b\u5973\u306e\u70ba\u306e\u30bb\u30d7\u30c6\u30c3\u30c8"),
        QString::fromWCharArray(L"\u9b54\u6cd5\u5c11\u5973\u9054\u306e\u767e\u5e74\u796d"),
        QString::fromWCharArray(L"U.N.\u30aa\u30fc\u30a8\u30f3\u306f\u5f7c\u5973\u306a\u306e\u304b\uff1f"),
        QString::fromWCharArray(L"\u7d05\u3088\u308a\u511a\u3044\u6c38\u9060"),
        QString::fromWCharArray(L"\u7d05\u697c\u3000\u301c Eastern Dream..."),
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName(QString::fromWCharArray(L"\u7d05\u9b54\u90f7MD.DAT"));
    const QString WavName("bgm/th06_%1.wav");
    const QString PosName("th06_%1.pos");

    struct FileInfo
    {
        uint offset;
        uint size;
        int checksum;
        QString name;
    };

    bool checkAllFileExists(const QDir& programDirectory)
    {
        if (!programDirectory.exists(FileName))
            return false;

        for (uint i = 0; i < SongDataSize; ++i)
            if (!programDirectory.exists(WavName.arg(i + 1, 2, 10, QLatin1Char('0'))))
                return false;

        return true;
    }
}

namespace PBG3
{
    bool checkMagicNumber(QFile& file)
    {
        quint32 magicNumber;
        if (file.read(reinterpret_cast<char*>(&magicNumber), 4) != 4)
            return false;
        if (qFromLittleEndian(magicNumber) != 0x33474250) //PBG3
            return false;
        return true;
    }

    bool getHeaderDescription(QFile& file, uint& maxFileCount, uint& headerPos, uint& headerSize)
    {
        QByteArray buffer = file.read(9);
        if (buffer.size() != 9)
            return false;
        BitReader reader(buffer);
        maxFileCount = reader.getUInt32();
        headerPos = reader.getUInt32();
        if (file.size() <= headerPos)
            return false;
        headerSize = file.size() - headerPos;
        return true;
    }

    bool getFileDescription(QFile& file, const uint maxFileCount, const uint headerPos, const uint headerSize, QList<FileInfo>& infoList)
    {
        file.seek(headerPos);
        QByteArray header = file.read(headerSize);
        BitReader reader(header);
        for (uint i = 0; i < maxFileCount; ++i)
        {
            FileInfo info;
            reader.getUInt32(); //time
            reader.getUInt32(); //time
            info.checksum = reader.getUInt32();
            info.offset = reader.getUInt32();
            info.size = reader.getUInt32();
            char c;
            while ( (c = reader.getChar()) )
                info.name += c;
            infoList << info;
        }
        // dummy info
        FileInfo info;
        info.offset = headerPos;
        infoList << info;
        return true;
    }

    bool parser(const QString& filename, QList<MusicInfo>& musicInfoList)
    {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly))
            return false;
        if (!checkMagicNumber(file))
            return false;

        uint maxFileCount;
        uint headerPos;
        uint headerSize;
        if (!getHeaderDescription(file, maxFileCount, headerPos, headerSize))
            return false;

        QList<FileInfo> infoList;
        if (!getFileDescription(file, maxFileCount, headerPos, headerSize, infoList))
            return false;

        QHash<QString, QByteArray> fileDataHash;
        for (int i = 0; i < infoList.size() - 1; ++i)
        {
            const FileInfo& info = infoList.at(i);
            if (!info.name.endsWith(".pos"))
                continue;
            file.seek(info.offset);
            uint compressedSize = infoList.at(i + 1).offset - info.offset;
            int checksum;
            QByteArray fileData = lzDecompressChecksum(checksum, file.read(compressedSize), info.size);
            Q_ASSERT(fileData.size() == info.size);
            Q_ASSERT(checksum == info.checksum);
            fileDataHash.insert(info.name, fileData);
        }

        for (uint i = 0; i < SongDataSize; ++i)
        {
            QString posName = PosName.arg(i + 1, 2, 10, QLatin1Char('0'));
            Q_ASSERT(fileDataHash.contains(posName));
            const uchar *d = reinterpret_cast<const uchar*>(fileDataHash.value(posName).data());
            MusicInfo musicInfo = {qFromLittleEndian<qint32>(d), qFromLittleEndian<qint32>(d + 4)};
            musicInfoList.append(musicInfo);
        }
        return true;
    }
}

Th06Loader::Th06Loader()
{
}

const QString& Th06Loader::title() const
{
    return Title;
}

uint Th06Loader::size() const
{
    return SongDataSize;
}

bool Th06Loader::open(const QString &path)
{
    programDirectory = QDir(path);
    if (!checkAllFileExists(programDirectory))
        return false;

    if (!PBG3::parser(programDirectory.filePath(FileName), musicInfoList))
        return false;
    return true;
}

MusicData Th06Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    const MusicInfo& musicInfo = musicInfoList.at(index);
    QFile file(programDirectory.absoluteFilePath(WavName.arg(index + 1, 2, 10, QLatin1Char('0'))));

    return MusicData(
        file.fileName(),
        SongData[index],
        "ZUN",
        Title,
        index + 1,
        SongDataSize,
        ".wav",
        file.size(),
        true,
        musicInfo.loopBegin,
        musicInfo.loopEnd
    );
}

void Th06Loader::close()
{
    musicInfoList.clear();
}
