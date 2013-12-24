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

#include "th075loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th075Loader", Th075Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u8403\u5922\u60f3\u3000\u301c Immaterial and Missing Power");
    const QString SongData[][2] = {
        {"sys00_op", QString::fromWCharArray(L"\u8403\u5922\u60f3")},
        {"00a", QString::fromWCharArray(L"\u6771\u65b9\u5996\u604b\u8ac7")},
        {"00b", QString::fromWCharArray(L"\u5c11\u5973\u7dba\u60f3\u66f2\u3000\u301c Capriccio")},
        {"01a", QString::fromWCharArray(L"\u604b\u8272\u30de\u30b8\u30c3\u30af")},
        {"01b", QString::fromWCharArray(L"\u9b54\u5973\u9054\u306e\u821e\u8e0f\u4f1a")},
        {"02a", QString::fromWCharArray(L"\u30e1\u30a4\u30c9\u3068\u8840\u306e\u61d0\u4e2d\u6642\u8a08")},
        {"02b", QString::fromWCharArray(L"\u6708\u6642\u8a08\u3000\u301c \u30eb\u30ca\u30fb\u30c0\u30a4\u30a2\u30eb")},
        {"03a", QString::fromWCharArray(L"\u30d6\u30af\u30ec\u30b7\u30e5\u30c6\u30a3\u306e\u4eba\u5f62\u5e2b")},
        {"03b", QString::fromWCharArray(L"\u4eba\u5f62\u88c1\u5224")},
        {"04a", QString::fromWCharArray(L"\u30e9\u30af\u30c8\u30ac\u30fc\u30eb\u3000\u301c \u5c11\u5973\u5bc6\u5ba4")},
        {"04b", QString::fromWCharArray(L"\u30f4\u30ef\u30eb\u9b54\u6cd5\u56f3\u66f8\u9928")},
        {"05a", QString::fromWCharArray(L"\u5e83\u6709\u5c04\u602a\u9ce5\u4e8b\u3000\u301c Till When\uff1f")},
        {"05b", QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Ancient Temple")},
        {"06a", QString::fromWCharArray(L"\u4ea1\u304d\u738b\u5973\u306e\u70ba\u306e\u30bb\u30d7\u30c6\u30c3\u30c8")},
        {"07a", QString::fromWCharArray(L"\u5e7d\u96c5\u306b\u54b2\u304b\u305b\u3001\u58a8\u67d3\u306e\u685c\u3000\u301c Border of Life")},
        {"00c", QString::fromWCharArray(L"Demystify Feast")},
        {"08a", QString::fromWCharArray(L"\u591c\u304c\u964d\u308a\u3066\u304f\u308b\u3000\u301c Evening Star")},
        {"09a", QString::fromWCharArray(L"\u5fa1\u4f3d\u306e\u56fd\u306e\u9b3c\u304c\u5cf6\u3000\u301c Missing Power")},
        {"53", QString::fromWCharArray(L"\u590f\u660e\u304d")},
        {"sys99_ed", QString::fromWCharArray(L"\u6771\u65b9\u8403\u5922\u60f3")},
        {"51", QString::fromWCharArray(L"\u9b54\u6240")},
        {"52", QString::fromWCharArray(L"\u6708\u8f2a")},
        {"54", QString::fromWCharArray(L"\u904d\u53c2")},
        {"56", QString::fromWCharArray(L"\u88cf\u5fc3")},
        {"57", QString::fromWCharArray(L"Intermezzo")},
        {"58", QString::fromWCharArray(L"\u3042\u3086\u306e\u304b\u305c")},
        {"67", QString::fromWCharArray(L"\u68ee\u9591")},
        {"62", QString::fromWCharArray(L"\u4ef0\u7a7a")},
        {"59", QString::fromWCharArray(L"\u5e7d\u5883")},
        {"68", QString::fromWCharArray(L"\u73cd\u5ba2")},
        {"60", QString::fromWCharArray(L"\u7d05\u591c")},
        {"61", QString::fromWCharArray(L"\u6226\u8fc5")},
        {"65", QString::fromWCharArray(L"\u798d\u6a5f")},
        {"63", QString::fromWCharArray(L"\u7815\u6708")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th075bgm.dat");
    const QString WavName("wave\\bgm\\%1.wav");

    struct FileDescription
    {
        char name[100];
        quint32 size;
        quint32 offset;
    };

    void decode(QByteArray& data, quint8 mask_init, quint8 mask_step, quint8 mask_step_step)
    {
        for (int i = 0; i < data.size(); ++i)
        {
            data.data()[i] ^= mask_init;
            mask_init += mask_step;
            mask_step += mask_step_step;
        }
    }
}

const QString& Th075Loader::title() const
{
    return Title;
}

uint Th075Loader::size() const
{
    return SongDataSize;
}

bool Th075Loader::open(const QString &path)
{
    dir = QDir(path);
    if (!dir.exists(FileName))
        return false;

// dat file parser
    QFile file(dir.absoluteFilePath(FileName));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    quint16 file_count;
    int header_size;
    {
        if (file.read(reinterpret_cast<char*>(&file_count), 2) != 2)
            return false;

        file_count = qFromLittleEndian(file_count);
        header_size = file_count * sizeof(FileDescription);
    }

    //qDebug() << file_count << header_size;

    QHash<QString, FileInfo> info_hash;
    {
        QByteArray header = file.read(header_size);
        if (header.size() != header_size)
            return false;

        decode(header, 0x64, 0x64, 0x4d);

        // parse header
        FileDescription* fileTable = reinterpret_cast<FileDescription*>(header.data());
        for (int i = 0; i < file_count; ++i)
        {
            FileInfo info;

            info.loopBegin = info.loopEnd = 0;
            info.offset = qFromLittleEndian<quint32>(fileTable[i].offset);
            info.size = qFromLittleEndian<quint32>(fileTable[i].size);
            QString name = QString::fromAscii(fileTable[i].name);

            info_hash.insert(name, info);
        }
    }

    enum
    {
        HAVE_RIFF  = 0x01,
        HAVE_WAVE  = 0x02,
        HAVE_fmt   = 0x04,
        HAVE_fact  = 0x08,
        HAVE_PEAK  = 0x10,
        HAVE_data  = 0x20,
        HAVE_cue   = 0x40,
        HAVE_LIST  = 0x80,
        HAVE_adtl  = 0x100,
        HAVE_ltxt  = 0x200,
        HAVE_labl  = 0x400,
        HAVE_rgn   = 0x800,
        HAVE_other = 0x80000000,
    };

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    #define MAKE_MARKER(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    #define MAKE_MARKER(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#endif
    enum
    {
        RIFF_MARKER = MAKE_MARKER('R','I','F','F'),
        WAVE_MARKER = MAKE_MARKER('W','A','V','E'),
        fmt_MARKER  = MAKE_MARKER('f','m','t',' '),
        data_MARKER = MAKE_MARKER('d','a','t','a'),
        cue_MARKER  = MAKE_MARKER('c','u','e',' '),
        LIST_MARKER = MAKE_MARKER('L','I','S','T'),
        adtl_MARKER = MAKE_MARKER('a','d','t','l'),
        ltxt_MARKER = MAKE_MARKER('l','t','x','t'),
        labl_MARKER = MAKE_MARKER('l','a','b','l'),
        rgn_MARKER  = MAKE_MARKER('r','g','n',' '),
    };
#undef MAKE_MARKER

    for (uint i = 0; i < SongDataSize; ++i)
    {
        QString wav = WavName.arg(SongData[i][0]);
        //qDebug() << wav;
        if (!info_hash.contains(wav))
            return false;
        FileInfo info = info_hash.value(wav);

// SoundForge sfl file parser
        if (!SFLParser(file, info.offset, info.size, info.loopBegin, info.loopEnd))
            return false;

        if (info.loopBegin == 0) // for sys99_ed
            info.loopEnd -= (181535 - 88200); // max:181535, wait 1 second
        info_list << info;
    }
    return true;
}

MusicData Th075Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list[index];
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(FileName), info.offset, info.offset + info.size);

    return MusicData(
        SongData[index][0] + ".wav",
        SongData[index][1],
        Title,
        ".wav",
        info.size,
        true,
        info.loopBegin,
        info.loopEnd,
        &archiveMusicData
    );
}

QByteArray Th075Loader::content(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list[index];

    QFile file(dir.absoluteFilePath(FileName));
    file.open(QIODevice::ReadOnly);

    file.seek(info.offset);
    return file.read(info.size);
}

void Th075Loader::close()
{
    info_list.clear();
}
