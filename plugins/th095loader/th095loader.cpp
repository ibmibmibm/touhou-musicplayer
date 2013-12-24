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

#include "th095loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th095Loader", Th095Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u6587\u82b1\u5e16\u3000\u301c Shoot the Bullet.");
    const QString SongData[][2] = {
        {"5_00", QString::fromWCharArray(L"\u5929\u72d7\u306e\u624b\u5e16\u3000\u301c Mysterious Note")},
        {"5_01", QString::fromWCharArray(L"\u98a8\u306e\u5faa\u74b0\u3000\u301c Wind Tour")},
        {"5_02", QString::fromWCharArray(L"\u5929\u72d7\u304c\u898b\u3066\u3044\u308b\u3000\u301c Black Eyes")},
        {"5_03", QString::fromWCharArray(L"\u6771\u306e\u56fd\u306e\u7720\u3089\u306a\u3044\u591c")},
        {"5_04", QString::fromWCharArray(L"\u30ec\u30c8\u30ed\u30b9\u30da\u30af\u30c6\u30a3\u30d6\u4eac\u90fd")},
        {"_08_2", QString::fromWCharArray(L"\u98a8\u795e\u5c11\u5973")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th095.dat");
    const QString BgmName("thbgm.dat");
    const QString WavName("th09%1.wav");

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

const QString& Th095Loader::title() const
{
    return Title;
}

uint Th095Loader::size() const
{
    return SongDataSize;
}

bool Th095Loader::open(const QString &path)
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
        /*uint thbgm_key;*/
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
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }
            /*thbgm_key = ('t'+'h'+'b'+'g'+'m'+'.'+'f'+'m'+'t') & 0x7;*/
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
        /*thbgm_data = decode(thbgm_data, KeyData[thbgm_key][0], thbgm_csize, KeyData[thbgm_key][1], KeyData[thbgm_key][2], KeyData[thbgm_key][3]);*/
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

MusicData Th095Loader::at(uint index)
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

void Th095Loader::close()
{
    info_hash.clear();
}
