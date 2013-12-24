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

#include "th10loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th10Loader", Th10Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u98a8\u795e\u9332\u3000\u301c Mountain of Faith.");
    const QString SongData[][2] = {
        {"02", QString::fromWCharArray(L"\u5c01\u5370\u3055\u308c\u3057\u795e\u3005")},
        {"00", QString::fromWCharArray(L"\u4eba\u604b\u3057\u795e\u69d8\u3000\u301c Romantic Fall")},
        {"01", QString::fromWCharArray(L"\u7a32\u7530\u59eb\u69d8\u306b\u53f1\u3089\u308c\u308b\u304b\u3089")},
        {"03", QString::fromWCharArray(L"\u5384\u795e\u69d8\u306e\u901a\u308a\u9053\u3000\u301c Dark Road")},
        {"04", QString::fromWCharArray(L"\u904b\u547d\u306e\u30c0\u30fc\u30af\u30b5\u30a4\u30c9")},
        {"05", QString::fromWCharArray(L"\u795e\u3005\u304c\u604b\u3057\u305f\u5e7b\u60f3\u90f7")},
        {"06", QString::fromWCharArray(L"\u82a5\u5ddd\u9f8d\u4e4b\u4ecb\u306e\u6cb3\u7ae5\u3000\u301c Candid Friend")},
        {"07", QString::fromWCharArray(L"\u30d5\u30a9\u30fc\u30eb\u30aa\u30d6\u30d5\u30a9\u30fc\u30eb\u3000\u301c \u79cb\u3081\u304f\u6edd")},
        {"08", QString::fromWCharArray(L"\u5996\u602a\u306e\u5c71\u3000\u301c Mysterious Mountain")},
        {"09", QString::fromWCharArray(L"\u5c11\u5973\u304c\u898b\u305f\u65e5\u672c\u306e\u539f\u98a8\u666f")},
        {"10", QString::fromWCharArray(L"\u4fe1\u4ef0\u306f\u511a\u304d\u4eba\u9593\u306e\u70ba\u306b")},
        {"11", QString::fromWCharArray(L"\u5fa1\u67f1\u306e\u5893\u5834\u3000\u301c Grave of Being")},
        {"12", QString::fromWCharArray(L"\u795e\u3055\u3073\u305f\u53e4\u6226\u5834\u3000\u301c Suwa Foughten Field")},
        {"15", QString::fromWCharArray(L"\u660e\u65e5\u30cf\u30ec\u306e\u65e5\u3001\u30b1\u306e\u6628\u65e5")},
        {"16", QString::fromWCharArray(L"\u30cd\u30a4\u30c6\u30a3\u30d6\u30d5\u30a7\u30a4\u30b9")},
        {"13", QString::fromWCharArray(L"\u9e93\u306e\u795e\u793e")},
        {"14", QString::fromWCharArray(L"\u795e\u306f\u6075\u307f\u306e\u96e8\u3092\u964d\u3089\u3059\u3000\u301c Sylphid Dream")},
        {"17", QString::fromWCharArray(L"\u30d7\u30ec\u30a4\u30e4\u30fc\u30ba\u30b9\u30b3\u30a2")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th10.dat");
    const QString BgmName("thbgm.dat");
    const QString WavName("th10_%1.wav");

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

const QString& Th10Loader::title() const
{
    return Title;
}

uint Th10Loader::size() const
{
    return SongDataSize;
}

bool Th10Loader::open(const QString &path)
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
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }
            thbgm_key = ('t'+'h'+'b'+'g'+'m'+'.'+'f'+'m'+'t') & 0x7;
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

MusicData Th10Loader::at(uint index)
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

QByteArray Th10Loader::content(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_hash.value(WavName.arg(SongData[index][0]));
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

void Th10Loader::close()
{
    info_hash.clear();
}
