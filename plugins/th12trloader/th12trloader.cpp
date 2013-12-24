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

#include "th12trloader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th12TrLoader", Th12TrLoader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u661f\u84ee\u8239\u3000\u301c Undefined Fantastic Object.");
    const QString SongData[][2] = {
        {"01", QString::fromWCharArray(L"\u9752\u7a7a\u306e\u5f71")},
        {"00", QString::fromWCharArray(L"\u6625\u306e\u6e4a\u306b")},
        {"02", QString::fromWCharArray(L"\u5c0f\u3055\u306a\u5c0f\u3055\u306a\u8ce2\u5c06")},
        {"04", QString::fromWCharArray(L"\u9589\u3056\u305b\u3057\u96f2\u306e\u901a\u3044\u8def")},
        {"05", QString::fromWCharArray(L"\u4e07\u5e74\u7f6e\u304d\u5098\u306b\u3054\u6ce8\u610f\u3092")},
        {"07", QString::fromWCharArray(L"\u30b9\u30ab\u30a4\u30eb\u30fc\u30a4\u30f3")},
        {"08", QString::fromWCharArray(L"\u6642\u4ee3\u89aa\u7236\u3068\u30cf\u30a4\u30ab\u30e9\u5c11\u5973")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th12tr.dat");
    const QString BgmName("thbgm_tr.dat");
    const QString WavName("th12_%1.wav");

    struct THA1PreHeader
    {
        quint32 magicNumber;
        quint32 headerOriginalSize;
        quint32 headerCompressedSize;
        quint32 maxFileCount;
    };

    const int KeyData[8][4] =
    {
        {0x1b, 0x73, 0x40, 0x3800},
        {0x51, 0x9e, 0x40, 0x4000},
        {0xc1, 0x15, 0x400, 0x2c00},
        {0x03, 0x91, 0x80, 0x6400},
        {0xab, 0xdc, 0x80, 0x6e00},
        {0x12, 0x43, 0x200, 0x3c00},
        {0x35, 0x79, 0x400, 0x3c00},
        {0x99, 0x7d, 0x80, 0x2800},
    };

    QByteArray decode(const QByteArray& ciphertext, char mask_init, int, char mask_step, int remix_step, int len)
    {
        int size = ciphertext.size();
        len -= (len % remix_step) + (len & 1);
        QByteArray plaintext(qMin(len, size), '\0');
        char mask = mask_init;
        int read_cursor = 0;
        int write_cursor = 0;
        for (int j = plaintext.size(); j > 0; j -= remix_step)
        {
            if (remix_step > j)
                remix_step = j;
            int write_cursor_copy = write_cursor + remix_step;
            write_cursor = write_cursor_copy - 1;
            for (int i = ((remix_step + 1) >> 1) ; i > 0; --i)
            {
                plaintext[write_cursor] = ciphertext[read_cursor] ^ mask;
                mask += mask_step;
                ++read_cursor;
                write_cursor -= 2;
            }
            write_cursor = write_cursor_copy - 2;
            for (int i = (remix_step >> 1) ; i > 0; --i)
            {
                plaintext[write_cursor] = ciphertext[read_cursor] ^ mask;
                mask += mask_step;
                ++read_cursor;
                write_cursor -= 2;
            }
            write_cursor = write_cursor_copy;
        }
        plaintext.append(ciphertext.mid(plaintext.size()));
        return plaintext;
    }
}

const QString& Th12TrLoader::title() const
{
    return Title;
}

uint Th12TrLoader::size() const
{
    return SongDataSize;
}

bool Th12TrLoader::open(const QString &path)
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
    QByteArray thbgm_data;
    {
// Stage 2
        header_pos = file.size() - header_csize;
        file.seek(header_pos);
        QByteArray header = lzDecompress(decode(file.read(header_csize), 0x3e, header_csize, 0x9b, 0x80, header_csize));
        char* cursor = header.data();
        uint thbgm_key;
        uint thbgm_offset;
        uint thbgm_dsize;
        uint thbgm_csize;
        for (uint i = 0; ; ++i)
        {
            if (i == max_file_count)
                return false;
            QLatin1String name(cursor);
            {
                size_t s = qstrlen(cursor) + 1;
                cursor += s + (-s & 3);
            }
            if (name != "thbgm_tr.fmt")
            {
                cursor += 12;
                continue;
            }
            thbgm_key = ('t'+'h'+'b'+'g'+'m'+'_'+'t'+'r'+'.'+'f'+'m'+'t') & 0x7;
            thbgm_offset = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)); //offset
            cursor += 4;
            thbgm_dsize = qFromLittleEndian<qint64>(reinterpret_cast<uchar*>(cursor)); //size
            cursor += 8;
            if (cursor < (header.data() + header.size()))
            {
                size_t s = qstrlen(cursor) + 1;
                cursor += s + (-s & 3);
                thbgm_csize = qFromLittleEndian<qint32>(reinterpret_cast<uchar*>(cursor)) - thbgm_offset;
            }
            else
                thbgm_csize = header_pos - thbgm_offset;
            break;
        }
        file.seek(thbgm_offset);
        thbgm_data = file.read(thbgm_csize);
        thbgm_data = decode(thbgm_data, KeyData[thbgm_key][0], thbgm_csize, KeyData[thbgm_key][1], KeyData[thbgm_key][2], KeyData[thbgm_key][3]);
        if (thbgm_csize != thbgm_dsize)
            thbgm_data = lzDecompress(thbgm_data, thbgm_dsize);
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

MusicData Th12TrLoader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_hash.value(WavName.arg(SongData[index][0]));
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(BgmName), info.offset, info.offset + info.size);

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

void Th12TrLoader::close()
{
    info_hash.clear();
}
