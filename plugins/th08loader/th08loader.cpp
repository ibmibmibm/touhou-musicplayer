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

#include "th08loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th08Loader", Th08Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u6c38\u591c\u6284\u3000\u301c Imperishable Night.");
    const QString SongData[][2] = {
        {"01", QString::fromWCharArray(L"\u6c38\u591c\u6284\u3000\u301c Eastern Night.")},
        {"00", QString::fromWCharArray(L"\u5e7b\u8996\u306e\u591c\u3000\u301c Ghostly Eyes")},
        {"03", QString::fromWCharArray(L"\u8822\u3005\u79cb\u6708\u3000\u301c Mooned Insect")},
        {"04", QString::fromWCharArray(L"\u591c\u96c0\u306e\u6b4c\u58f0\u3000\u301c Night Bird")},
        {"05", QString::fromWCharArray(L"\u3082\u3046\u6b4c\u3057\u304b\u805e\u3053\u3048\u306a\u3044")},
        {"06", QString::fromWCharArray(L"\u61d0\u304b\u3057\u304d\u6771\u65b9\u306e\u8840\u3000\u301c Old World")},
        {"07", QString::fromWCharArray(L"\u30d7\u30ec\u30a4\u30f3\u30a8\u30a4\u30b8\u30a2")},
        {"08", QString::fromWCharArray(L"\u6c38\u591c\u306e\u5831\u3044\u3000\u301c Imperishable Night.")},
        {"09", QString::fromWCharArray(L"\u5c11\u5973\u7dba\u60f3\u66f2\u3000\u301c Dream Battle")},
        {"10", QString::fromWCharArray(L"\u604b\u8272\u30de\u30b9\u30bf\u30fc\u30b9\u30d1\u30fc\u30af")},
        {"11", QString::fromWCharArray(L"\u30b7\u30f3\u30c7\u30ec\u30e9\u30b1\u30fc\u30b8\u3000\u301c Kagome-Kagome")},
        {"12", QString::fromWCharArray(L"\u72c2\u6c17\u306e\u77b3\u3000\u301c Invisible Full Moon")},
        {"13", QString::fromWCharArray(L"\u30f4\u30a9\u30e4\u30fc\u30b8\u30e51969")},
        {"14", QString::fromWCharArray(L"\u5343\u5e74\u5e7b\u60f3\u90f7\u3000\u301c History of the Moon")},
        {"15", QString::fromWCharArray(L"\u7af9\u53d6\u98db\u7fd4\u3000\u301c Lunatic Princess")},
        {"13b", QString::fromWCharArray(L"\u30f4\u30a9\u30e4\u30fc\u30b8\u30e51970")},
        {"18", QString::fromWCharArray(L"\u30a8\u30af\u30b9\u30c6\u30f3\u30c9\u30a2\u30c3\u30b7\u30e5\u3000\u301c \u84ec\u83b1\u4eba")},
        {"19", QString::fromWCharArray(L"\u6708\u307e\u3067\u5c4a\u3051\u3001\u4e0d\u6b7b\u306e\u7159")},
        {"16", QString::fromWCharArray(L"\u6708\u898b\u8349")},
        {"17", QString::fromWCharArray(L"Eternal Dream\u3000\u301c \u5e7d\u7384\u306e\u69ed\u6a39")},
        {"20", QString::fromWCharArray(L"\u6771\u65b9\u5996\u602a\u5c0f\u753a")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th08.dat");
    const QString BgmName("thbgm.dat");
    const QString WavName("th08_%1.wav");

    struct PGBXPreHeader
    {
        quint32 maxFileCount;
        quint32 headerPos;
        quint32 headerDictsize;
    };

    QByteArray decode(const QByteArray& ciphertext, char mask_init, char mask_step, int remix_step, int len)
    {
        int size = ciphertext.size();
        QByteArray plaintext(qMin(len, size), '\0');
        plaintext.append(ciphertext.mid(len, size - len));
        char mask = mask_init;
        int read_cursor = 0;
        int write_cursor = 0;
        for (int j = size & ~1; j > 0 && len > 0; j -= remix_step, len -= remix_step)
        {
            if (j < remix_step)
                remix_step = j;
            int write_cursor_copy = write_cursor;
            write_cursor = write_cursor_copy + remix_step - 1;
            for (int i = (remix_step >> 1) ; i > 0; --i)
            {
                plaintext[write_cursor] = ciphertext[read_cursor] ^ mask;
                ++read_cursor;
                write_cursor -= 2;
                mask += mask_step;
            }
            write_cursor = write_cursor_copy + remix_step - 2;
            for (int i = (remix_step >> 1) ; i > 0; --i)
            {
                plaintext[write_cursor] = ciphertext[read_cursor] ^ mask;
                ++read_cursor;
                write_cursor -= 2;
                mask += mask_step;
            }
            write_cursor = write_cursor_copy + remix_step;
        }
        return plaintext;
    }
}

const QString& Th08Loader::title() const
{
    return Title;
}

uint Th08Loader::size() const
{
    return SongDataSize;
}

bool Th08Loader::open(const QString &path)
{
    dir = QDir(path);
    if (!dir.exists(FileName) || !dir.exists(BgmName))
        return false;

// PBGX decompresser
    QFile file(dir.filePath(FileName));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    uint max_file_count;
    uint header_pos;
    uint header_csize;
    uint header_dictsize;
    {
        quint32 magicNumber;
        if (file.read(reinterpret_cast<char*>(&magicNumber), 4) != 4)
            return false;
        if (qFromLittleEndian(magicNumber) != 0x5a474250) //PBGX
            return false;
    }
// Stage 1
    {
        QByteArray preHeaderData = decode(file.read(12), 0x1b, 0x37, 0xc, 0x400);
        PGBXPreHeader* preHeader = reinterpret_cast<PGBXPreHeader*>(preHeaderData.data());
        max_file_count = qFromLittleEndian(preHeader->maxFileCount) - 123456;
        header_pos = qFromLittleEndian(preHeader->headerPos) - 345678;
        header_dictsize = qFromLittleEndian(preHeader->headerDictsize) - 567891;
        if (file.size() <= header_pos)
            return false;
        header_csize = file.size() - header_pos;
    }

    QByteArray thbgm_data;
    {
// Stage 2
        file.seek(header_pos);
        QByteArray header = lzDecompressDictSize(decode(file.read(header_csize), 0x3e, 0x9b, 0x80, 0x400), header_dictsize);
        char* cursor = header.data();
        uint thbgm_offset;
        uint thbgm_dsize;
        uint thbgm_csize;
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
        QByteArray thbgm_cdata = lzDecompress(file.read(thbgm_csize));
        QByteArray thbgm_magic = thbgm_cdata.left(4);
        thbgm_cdata = thbgm_cdata.mid(4);
        switch (thbgm_magic.data()[3])
        {
            case 'M':
                thbgm_data = decode(thbgm_cdata, 0x0, 0x0, 0x0, 0x0);
                break;
            case 'T':
                thbgm_data = decode(thbgm_cdata, 0x51, 0xe9, 0x40, 0x3000);
                break;
            case 'A':
                thbgm_data = decode(thbgm_cdata, 0xc1, 0x51, 0x1400, 0x2000);
                break;
            case 'J':
                thbgm_data = decode(thbgm_cdata, 0x03, 0x19, 0x1400, 0x7800);
                break;
            case 'E':
                thbgm_data = decode(thbgm_cdata, 0x0, 0x0, 0x0, 0x0);
                break;
            case 'W':
                thbgm_data = decode(thbgm_cdata, 0x12, 0x34, 0x400, 0x2800);
                break;
            case '-':
                thbgm_data = decode(thbgm_cdata, 0x35, 0x97, 0x80, 0x2800);
        }
    }
// Stage3
    {
        QList<FileInfo> info_list;
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
        foreach(FileInfo info, info_list)
        {
            info_hash.insert(info.name, info);
        }
    }

    QFile wav(dir.filePath(BgmName));
    if (!wav.open(QIODevice::ReadOnly))
        return false;
    return true;
}

MusicData Th08Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_hash.value(WavName.arg(SongData[index][0]));
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(BgmName), info.offset, info.offset + info.size);
    //qDebug() << info.name << Title;

    return MusicData(
        info.name,
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

void Th08Loader::close()
{
    info_hash.clear();
}
