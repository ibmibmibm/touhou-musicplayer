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

#include "alcoloader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("AlcoLoader", AlcoLoader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u9ec4\u660f\u9152\u5834\u3000\u301c Uwabami Breakers");
    const QString SongData[][3] = {
        {"01", QString::fromWCharArray(L"step u/b"), QString::fromWCharArray(L"\u9aa8\u6298\u98f2\u6599")},
        {"02", QString::fromWCharArray(L"chase the Twilight"), QString::fromWCharArray(L"\u9aa8\u6298\u98f2\u6599")},
        {"03", QString::fromWCharArray(L"\u5451\u3093\u3079\u3047\u306e\u30ec\u30e0\u30ea\u30a2"), QString::fromWCharArray(L"ZUN")},
        {"04", QString::fromWCharArray(L"Festiv-al-colohically"), QString::fromWCharArray(L"\u9aa8\u6298\u98f2\u6599")},
        {"05", QString::fromWCharArray(L"Valse Alcoholique"), QString::fromWCharArray(L"\u7fbd\u9ce5\u98a8\u753b")},
        {"06", QString::fromWCharArray(L"\u7d76\u5bfe\u5927\u4e08\u592b\u3060\u3088\uff01\uff01\u3044\u304f\u3089\u98f2\u3093\u3067\u3082\u671d\u304c\u6765\u308b\u3088\uff01\uff01"), QString::fromWCharArray(L"Kiyoma")},
        {"07", QString::fromWCharArray(L"step u/b (lazy style)"), QString::fromWCharArray(L"\u9aa8\u6298\u98f2\u6599")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("alcostg.dat");
    const QString BgmName("albgm.dat");
    const QString WavName("alcostg%1.wav");

    struct THA1PreHeader
    {
        quint32 magicNumber;
        quint32 headerOriginalSize;
        quint32 headerCompressedSize;
        quint32 maxFileCount;
    };

    const int KeyData[8][4] =
    {
        {0x1b, 0x37, 0x40, 0x2800},
        {0x51, 0xe9, 0x40, 0x3000},
        {0xc1, 0x51, 0x80, 0x3200},
        {0x03, 0x19, 0x400, 0x7800},
        {0xab, 0xcd, 0x200, 0x2800},
        {0x12, 0x34, 0x80, 0x3200},
        {0x35, 0x97, 0x80, 0x2800},
        {0x99, 0x37, 0x400, 0x2000},
    };

    QByteArray decode(const QByteArray& ciphertext, char mask_init, int, char mask_step, int remix_step, int len)
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
            int write_cursor_copy = write_cursor + remix_step;
            write_cursor = write_cursor_copy - 1;
            for (int i = (remix_step >> 1) ; i > 0; --i)
            {
                plaintext[write_cursor] = ciphertext[read_cursor] ^ mask;
                ++read_cursor;
                write_cursor -= 2;
                mask += mask_step;
            }
            write_cursor = write_cursor_copy - 2;
            for (int i = (remix_step >> 1) ; i > 0; --i)
            {
                plaintext[write_cursor] = ciphertext[read_cursor] ^ mask;
                ++read_cursor;
                write_cursor -= 2;
                mask += mask_step;
            }
            write_cursor = write_cursor_copy;
        }
        return plaintext;
    }
}

const QString& AlcoLoader::title() const
{
    return Title;
}

uint AlcoLoader::size() const
{
    return SongDataSize;
}

bool AlcoLoader::open(const QString &path)
{
    dir = QDir(path);
    if (!dir.exists(FileName) || !dir.exists(BgmName))
        return false;

// THA1 decompresser
    QFile file(dir.filePath(FileName));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    uint max_file_count;
    uint header_csize;
    uint header_dsize;
    uint header_pos;
// Stage 1
    {
        QByteArray preHeaderData = decode(file.read(0x10), 0x1b, 0x10, 0x37, 0x10, 0x10);
        THA1PreHeader* preHeader = reinterpret_cast<THA1PreHeader*>(preHeaderData.data());
        if (qFromLittleEndian(preHeader->magicNumber) != 0x31414854) // THA1
            return false;
        header_dsize = qFromLittleEndian(preHeader->headerOriginalSize) - 123456789;
        header_csize = qFromLittleEndian(preHeader->headerCompressedSize) - 987654321;
        max_file_count = qFromLittleEndian(preHeader->maxFileCount) - 135792468;
        if (file.size() <= header_csize + 0x10)
            return false;
    }

    QByteArray albgm_data;
    {
// Stage 2
        header_pos = file.size() - header_csize;
        file.seek(header_pos);
        QByteArray header = lzDecompress(decode(file.read(header_csize), 0x3e, header_csize, 0x9b, 0x80, header_csize));
        char* cursor = header.data();
        uint albgm_key;
        uint albgm_offset;
        uint albgm_dsize;
        uint albgm_csize;
        for (uint i = 0; ; ++i)
        {
            if (i == max_file_count)
                return false;
            QLatin1String name(cursor);
            {
                size_t s = qstrlen(cursor) + 1;
                cursor += s + (-s & 3);
            }
            if (name != "albgm.fmt")
            {
                cursor += 12;
                continue;
            }
            albgm_key = ('a'+'l'+'b'+'g'+'m'+'.'+'f'+'m'+'t') & 0x7;
            albgm_offset = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)); //offset
            cursor += 4;
            albgm_dsize = qFromLittleEndian<qint64>(reinterpret_cast<uchar*>(cursor)); //size
            cursor += 8;
            if (cursor < (header.data() + header.size()))
            {
                size_t s = qstrlen(cursor) + 1;
                cursor += s + (-s & 3);
                albgm_csize = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)) - albgm_offset;
            }
            else
                albgm_csize = header_pos - albgm_offset;
            break;
        }
        file.seek(albgm_offset);
        albgm_data = file.read(albgm_csize);
        albgm_data = decode(albgm_data, KeyData[albgm_key][0], albgm_csize, KeyData[albgm_key][1], KeyData[albgm_key][2], KeyData[albgm_key][3]);
        if (albgm_csize != albgm_dsize)
            albgm_data = lzDecompress(albgm_data, albgm_dsize);
    }
// Stage3
    {
        QList<FileInfo> info_list;
        ThbgmData* albgmData = reinterpret_cast<ThbgmData*>(albgm_data.data());
        for (uint i = 0; i < SongDataSize; ++i)
        {
            FileInfo info;
            info.name = albgmData[i].name;
            info.offset = qFromLittleEndian<qint32>(albgmData[i].offset);
            info.checksum = qFromLittleEndian<qint32>(albgmData[i].checksum);
            info.loopBegin = qFromLittleEndian<qint32>(albgmData[i].loopBegin) >> 2;
            info.loopEnd = qFromLittleEndian<qint32>(albgmData[i].loopEnd) >> 2;
            info.header = QByteArray(albgmData[i].header, 16);
            if (i)
                info_list[i - 1].size = info.offset - info_list[i - 1].offset;
            info_list << info;
        }
        info_list[SongDataSize - 1].size = QFileInfo(dir.filePath(BgmName)).size() - info_list[SongDataSize - 1].offset;
        info_list[SongDataSize - 1].loopEnd = info_list[SongDataSize - 1].size >> 2;
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

MusicData AlcoLoader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_hash.value(WavName.arg(SongData[index][0]));
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(BgmName), info.offset, info.offset + info.size);
    //qDebug() << info.name << info.loopBegin << info.loopEnd << info.size;

    return MusicData(
        info.name,
        SongData[index][1],
        SongData[index][2],
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

void AlcoLoader::close()
{
    info_hash.clear();
}
