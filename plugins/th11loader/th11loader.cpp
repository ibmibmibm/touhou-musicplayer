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
#include <QtDebug>
#include <QMessageBox>

#include "th11loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th11Loader", Th11Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u5730\u970a\u6bbf\u3000\u301c Subterranean Animism.");
    const QString SongData[][2] = {
        {"00", QString::fromWCharArray(L"\u5730\u970a\u9054\u306e\u8d77\u5e8a")},
        {"01", QString::fromWCharArray(L"\u6697\u95c7\u306e\u98a8\u7a74")},
        {"02", QString::fromWCharArray(L"\u5c01\u3058\u3089\u308c\u305f\u5996\u602a\u3000\u301c Lost Place")},
        {"03", QString::fromWCharArray(L"\u6e21\u308b\u8005\u306e\u9014\u7d76\u3048\u305f\u6a4b")},
        {"05", QString::fromWCharArray(L"\u7dd1\u773c\u306e\u30b8\u30a7\u30e9\u30b7\u30fc")},
        {"06", QString::fromWCharArray(L"\u65e7\u5730\u7344\u8857\u9053\u3092\u884c\u304f")},
        {"07", QString::fromWCharArray(L"\u83ef\u306e\u3055\u304b\u3065\u304d\u5927\u6c5f\u5c71")},
        {"08", QString::fromWCharArray(L"\u30cf\u30fc\u30c8\u30d5\u30a7\u30eb\u30c8\u30d5\u30a1\u30f3\u30b7\u30fc")},
        {"10", QString::fromWCharArray(L"\u5c11\u5973\u3055\u3068\u308a\u3000\u301c 3rd eye")},
        {"12", QString::fromWCharArray(L"\u5ec3\u7344\u30e9\u30e9\u30d0\u30a4")},
        {"13", QString::fromWCharArray(L"\u6b7b\u4f53\u65c5\u884c\u3000\u301c Be of good cheer!")},
        {"14", QString::fromWCharArray(L"\u696d\u706b\u30de\u30f3\u30c8\u30eb")},
        {"16", QString::fromWCharArray(L"\u970a\u77e5\u306e\u592a\u967d\u4fe1\u4ef0\u3000\u301c Nuclear Fusion")},
        {"15", QString::fromWCharArray(L"\u30e9\u30b9\u30c8\u30ea\u30e2\u30fc\u30c8")},
        {"17", QString::fromWCharArray(L"\u30cf\u30eb\u30c8\u30de\u30f3\u306e\u5996\u602a\u5c11\u5973")},
        {"18", QString::fromWCharArray(L"\u5730\u970a\u9054\u306e\u5e30\u5b85")},
        {"19", QString::fromWCharArray(L"\u30a8\u30cd\u30eb\u30ae\u30fc\u9ece\u660e \u301c Future Dream...")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th11.dat");
    const QString BgmName("thbgm.dat");
    const QString WavName("th11_%1.wav");

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

    class BitReader
    {
        public:
            BitReader(const char*const _data, const int _size) :
                pos(0),
                bitPos(0x80),
                size(_size),
                data(_data)
            {}
            bool getBit()
            {
                if (pos >= size)
                    return false;
                bool ret = (static_cast<unsigned char>(data[pos]) & bitPos);
                bitPos >>= 1;
                if (!bitPos)
                {
                    ++pos;
                    bitPos = 0x80;
                }
                return ret;
            }
            uint getBits(int len)
            {
                uint ret = 0;
                uint mask = 1 << --len;
                for (int i = 0; i <= len; ++i)
                {
                    if (getBit())
                        ret |= mask;
                    mask >>= 1;
                }
                return ret;
            }
            char getChar()
            {
                return getBits(8);
            }
        private:
            uint pos;
            quint8 bitPos;
            const uint size;
            const char*const data;
    };

    QByteArray decompress(const QByteArray& compressed)
    {
        QByteArray decompressd;
        QByteArray dict(0x2000, '\0');
        BitReader reader(compressed.data(), compressed.size());
        int dict_cursor = 1;
        do
        {
            if (reader.getBit())
            {
                char c = reader.getChar();
                decompressd.append(c);
                dict[dict_cursor] = c;
                dict_cursor = (dict_cursor + 1) & 0x1fff;
            }
            else
            {
                int addr = reader.getBits(13);
                if (addr == 0)
                    return decompressd;
                int jump = reader.getBits(4) + 3;
                for (int i = 0; i < jump; ++i)
                {
                    char c = dict[addr];
                    addr = (addr + 1) & 0x1fff;
                    decompressd.append(c);
                    dict[dict_cursor] = c;
                    dict_cursor = (dict_cursor + 1) & 0x1fff;
                }
            }
        } while (true);
    }

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

const QString& Th11Loader::title() const
{
    return Title;
}

uint Th11Loader::size() const
{
    return SongDataSize;
}

bool Th11Loader::open(const QString &path)
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
        QByteArray pre_header = decode(file.read(0x10), 0x1b, 0x10, 0x37, 0x10, 0x10);
        if (getUInt32(pre_header.data()) != 0x31414854) // THA1
            return false;
        header_dsize = getUInt32(pre_header.data() + 4) - 123456789;
        header_csize = getUInt32(pre_header.data() + 8) - 987654321;
        max_file_count = getUInt32(pre_header.data() + 12) - 135792468;
        if (file.size() <= header_csize + 0x10)
            return false;
    }

    QByteArray thbgm_data;
    {
// Stage 2
        header_pos = file.size() - header_csize;
        file.seek(header_pos);
        QByteArray header = decompress(decode(file.read(header_csize), 0x3e, header_csize, 0x9b, 0x80, header_csize));
        int cursor = 0;
        uint thbgm_key;
        uint thbgm_offset;
        uint thbgm_dsize;
        uint thbgm_csize;
        for (uint i = 0; ; ++i)
        {
            if (i == max_file_count)
                return false;
            QByteArray name(header.data() + cursor);
            cursor += name.size() + 1;
            while ((cursor & 3) != 0)
                ++cursor;
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }

            thbgm_key = 0;
            for (int i = 0; i < name.size(); ++i)
                thbgm_key += name.at(i);
            thbgm_key &= 0x7;

            thbgm_offset = getUInt32(header.data() + cursor);
            cursor += 4;
            thbgm_dsize = getUInt32(header.data() + cursor);
            cursor += 8;
            if (cursor < header.size())
            {
                QByteArray n(header.data() + cursor);
                int cu = cursor + n.size() + 1;
                while ((cu & 3) != 0)
                    ++cu;
                    thbgm_csize = getUInt32(header.data() + cu) - thbgm_offset;
            }
            else
                thbgm_csize = header_pos - thbgm_offset;
            break;
        }
        file.seek(thbgm_offset);
        thbgm_data = file.read(thbgm_csize);
        thbgm_data = decode(thbgm_data, KeyData[thbgm_key][0], thbgm_csize, KeyData[thbgm_key][1], KeyData[thbgm_key][2], KeyData[thbgm_key][3]);
        if (thbgm_csize != thbgm_dsize)
            thbgm_data = decompress(thbgm_data);
    }
// Stage3
    {
        QList<FileInfo> info_list;
        uint cursor = 0;
        for (uint i = 0; i < SongDataSize; ++i)
        {
            FileInfo info;
            info.name = thbgm_data.data() + cursor;
            cursor += 16;
            info.offset = getUInt32(thbgm_data.data() + cursor);
            cursor += 4;
            info.checksum = getUInt32(thbgm_data.data() + cursor);
            cursor += 4;
            info.loopBegin = getUInt32(thbgm_data.data() + cursor) >> 2;
            cursor += 4;
            info.loopEnd = getUInt32(thbgm_data.data() + cursor) >> 2;
            cursor += 4;
            info.header =  QByteArray(thbgm_data.data() + cursor, 16);
            cursor += 20;
            if (i)
                info_list[i - 1].size = info.offset - info_list[i - 1].offset;
            info_list << info;
        }
        info_list[SongDataSize - 1].size = 362242056 - info_list[SongDataSize - 1].offset;
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

MusicData Th11Loader::at(uint index)
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

QByteArray Th11Loader::content(uint index)
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

void Th11Loader::close()
{
    info_hash.clear();
}
