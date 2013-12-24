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

#include "th09loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th09Loader", Th09Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u82b1\u6620\u585a\u3000\u301c Phantasmagoria of Flower View.");
    const QString SongData[][2] = {
        {"09_00", QString::fromWCharArray(L"\u82b1\u6620\u585a\u3000\u301c Higan Retour")},
        {"09_01", QString::fromWCharArray(L"\u6625\u8272\u5c0f\u5f84\u3000\u301c Colorful Path")},
        {"09_00b", QString::fromWCharArray(L"\u30aa\u30ea\u30a8\u30f3\u30bf\u30eb\u30c0\u30fc\u30af\u30d5\u30e9\u30a4\u30c8")},
        {"09_02", QString::fromWCharArray(L"\u30d5\u30e9\u30ef\u30ea\u30f3\u30b0\u30ca\u30a4\u30c8")},
        {"07_10_b", QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Ancient Temple")},
        {"08_12", QString::fromWCharArray(L"\u72c2\u6c17\u306e\u77b3\u3000\u301c Invisible Full Moon")},
        {"09_05", QString::fromWCharArray(L"\u304a\u3066\u3093\u3070\u604b\u5a18\u306e\u5192\u967a")},
        {"07_09", QString::fromWCharArray(L"\u5e7d\u970a\u697d\u56e3\u3000\u301c Phantom Ensemble")},
        {"09_07", QString::fromWCharArray(L"\u3082\u3046\u6b4c\u3057\u304b\u805e\u3053\u3048\u306a\u3044\u3000\u301c Flower Mix")},
        {"09_10", QString::fromWCharArray(L"\u304a\u5b87\u4f50\u3055\u307e\u306e\u7d20\u3044\u5e61")},
        {"09_08_2", QString::fromWCharArray(L"\u98a8\u795e\u5c11\u5973   (Short Version)")},
        {"09_12", QString::fromWCharArray(L"\u30dd\u30a4\u30ba\u30f3\u30dc\u30c7\u30a3\u3000\u301c Forsaken Doll")},
        {"09_13", QString::fromWCharArray(L"\u4eca\u6614\u5e7b\u60f3\u90f7\u3000\u301c Flower Land")},
        {"09_09", QString::fromWCharArray(L"\u5f7c\u5cb8\u5e30\u822a\u3000\u301c Riverside View")},
        {"09_11", QString::fromWCharArray(L"\u516d\u5341\u5e74\u76ee\u306e\u6771\u65b9\u88c1\u5224\u3000\u301c Fate of Sixty Years")},
        {"09_00c", QString::fromWCharArray(L"\u82b1\u306e\u6620\u308b\u585a")},
        {"09_15", QString::fromWCharArray(L"\u6b64\u5cb8\u306e\u585a")},
        {"09_14", QString::fromWCharArray(L"\u82b1\u306f\u5e7b\u60f3\u306e\u307e\u307e\u306b")},
        {"09_17", QString::fromWCharArray(L"\u9b42\u306e\u82b1\u3000\u301c Another Dream...")},
    };
    const int SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th09.dat");
    const QString BgmName("thbgm.dat");
    const QString WavName("th%1.wav");

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
            uint getUInt32()
            {
                return getBits((getBits(2) + 1) << 3);
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

    QByteArray decompress(const QByteArray& compressed, int dict_size)
    {
        QByteArray decompressd;
        QByteArray dict(dict_size, '\0');
        BitReader reader(compressed.data(), compressed.size());
        int dict_cursor = 1;
        char c;
        int addr, jump;
        do
        {
            if (reader.getBit())
            {
                c = reader.getChar();
                decompressd.append(c);
                dict[dict_cursor] = c;
                dict_cursor = (dict_cursor + 1) % dict_size;
            }
            else
            {
                addr = reader.getBits(13);
                if (addr == 0)
                    return decompressd;
                jump = reader.getBits(4) + 3;
                for (int i = 0; i < jump; ++i)
                {
                    c = dict[addr];
                    addr = (addr + 1) % dict_size;
                    decompressd.append(c);
                    dict[dict_cursor] = c;
                    dict_cursor = (dict_cursor + 1) % dict_size;
                }
            }
        } while (true);
    }

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

const QString& Th09Loader::title() const
{
    return Title;
}

uint Th09Loader::size() const
{
    return SongDataSize;
}

bool Th09Loader::open(const QString &path)
{
    dir = QDir(path);
    if (!dir.exists(FileName) || !dir.exists(BgmName))
        return false;

// PBGX decompresser
    QFile file(dir.filePath(FileName));
    if (file.size() != 88180006)
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    uint max_file_count;
    uint header_pos;
    uint header_csize;
    uint header_dictsize;
    {
        char buffer[4];
        if (file.read(buffer, 4) != 4)
            return false;
        if (getUInt32(buffer) != 0x5a474250) //PBGX
            return false;
    }
// Stage 1
    {
        QByteArray pre_header = decode(file.read(12), 0x1b, 0x37, 0xc, 0x400);
        max_file_count = getUInt32(pre_header.data()) - 123456;
        header_pos = getUInt32(pre_header.data() + 4) - 345678;
        header_dictsize = getUInt32(pre_header.data() + 8) - 567891;
        if (file.size() <= header_pos)
            return false;
        header_csize = file.size() - header_pos;
    }

    QByteArray thbgm_ddata;
    {
// Stage 2
        file.seek(header_pos);
        QByteArray header = decompress(decode(file.read(header_csize), 0x3e, 0x9b, 0x80, 0x400), header_dictsize);
        uint cursor = 0;
        uint thbgm_offset;
        uint thbgm_dsize;
        uint thbgm_csize;
        for (uint i = 0; ; ++i)
        {
            if (i == max_file_count)
                return false;
            QString name;
            char c;
            while ( (c = header[cursor++]) )
                name += c;
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }
            thbgm_offset = getUInt32(header.data() + cursor);
            cursor += 4;
            thbgm_dsize = getUInt32(header.data() + cursor);
            cursor += 8;
            while ( (c = header[cursor++]) )
                ;
            thbgm_csize = getUInt32(header.data() + cursor) - thbgm_offset;
            break;
        }
        file.seek(thbgm_offset);
        QByteArray thbgm_cdata = decompress(file.read(thbgm_csize), 0x2000);
        QByteArray thbgm_magic = thbgm_cdata.left(4);
        thbgm_cdata = thbgm_cdata.mid(4);
        switch (thbgm_magic.data()[3])
        {
            case 'M':
                thbgm_ddata = decode(thbgm_cdata, 0x0, 0x0, 0x0, 0x0);
                break;
            case 'T':
                thbgm_ddata = decode(thbgm_cdata, 0x51, 0xe9, 0x40, 0x3000);
                break;
            case 'A':
                thbgm_ddata = decode(thbgm_cdata, 0xc1, 0x51, 0x1400, 0x2000);
                break;
            case 'J':
                thbgm_ddata = decode(thbgm_cdata, 0x03, 0x19, 0x1400, 0x7800);
                break;
            case 'E':
                thbgm_ddata = decode(thbgm_cdata, 0x0, 0x0, 0x0, 0x0);
                break;
            case 'W':
                thbgm_ddata = decode(thbgm_cdata, 0x12, 0x34, 0x400, 0x2800);
                break;
            case '-':
                thbgm_ddata = decode(thbgm_cdata, 0x35, 0x97, 0x80, 0x2800);
        }
    }
// Stage3
    {
        QList<FileInfo> info_list;
        uint cursor = 0;
        for (int i = 0; i < SongDataSize; ++i)
        {
            FileInfo info;
            info.name = thbgm_ddata.data() + cursor;
            cursor += 16;
            info.offset = getUInt32(thbgm_ddata.data() + cursor);
            cursor += 4;
            info.checksum = getUInt32(thbgm_ddata.data() + cursor);
            cursor += 4;
            info.loopStart = getUInt32(thbgm_ddata.data() + cursor) >> 2;
            cursor += 4;
            info.loopEnd = getUInt32(thbgm_ddata.data() + cursor) >> 2;
            cursor += 4;
            info.header =  QByteArray(thbgm_ddata.data() + cursor, 16);
            cursor += 20;
            if (i)
                info_list[i - 1].size = info.offset - info_list[i - 1].offset;
            info_list << info;
        }
        info_list[SongDataSize - 1].size = 449961024 - info_list[SongDataSize - 1].offset;
        foreach(FileInfo info, info_list)
        {
            info_hash.insert(info.name, info);
        }
    }

    QFile wav(dir.filePath(BgmName));
    if (wav.size() != 439835760)
        return false;
    if (!wav.open(QIODevice::ReadOnly))
        return false;
    return true;
}

MusicData Th09Loader::at(uint index)
{
    Q_ASSERT(0 <= index && index < SongDataSize);
    FileInfo info = info_hash.value(WavName.arg(SongData[index][0]));
//    qDebug() << info.name << Title;

    return MusicData(
        ".wav",
        SongData[index][1],
        Title,
        info.size + 44,
        true,
        info.loopStart,
        info.loopEnd
    );
}

QByteArray Th09Loader::content(uint index)
{
    Q_ASSERT(0 <= index && index < SongDataSize);
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

void Th09Loader::close()
{
    info_hash.clear();
}
