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

#include "th07loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th07Loader", Th07Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Perfect Cherry Blossom.");
    const QString SongData[] = {
        QString::fromWCharArray(L"\u5996\u3005\u5922\u3000\u301c Snow or Cherry Petal"),
        QString::fromWCharArray(L"\u7121\u4f55\u6709\u306e\u90f7\u3000\u301c Deep Mountain"),
        QString::fromWCharArray(L"\u30af\u30ea\u30b9\u30bf\u30e9\u30a4\u30ba\u30b7\u30eb\u30d0\u30fc"),
        QString::fromWCharArray(L"\u9060\u91ce\u5e7b\u60f3\u7269\u8a9e"),
        QString::fromWCharArray(L"\u30c6\u30a3\u30a2\u30aa\u30a4\u30a8\u30c4\u30a9\u30f3(withered leaf)"),
        QString::fromWCharArray(L"\u30d6\u30af\u30ec\u30b7\u30e5\u30c6\u30a3\u306e\u4eba\u5f62\u5e2b"),
        QString::fromWCharArray(L"\u4eba\u5f62\u88c1\u5224\u3000\u301c \u4eba\u306e\u5f62\u5f04\u3073\u3057\u5c11\u5973"),
        QString::fromWCharArray(L"\u5929\u7a7a\u306e\u82b1\u306e\u90fd"),
        QString::fromWCharArray(L"\u5e7d\u970a\u697d\u56e3\u3000\u301c Phantom Ensemble"),
        QString::fromWCharArray(L"\u6771\u65b9\u5996\u3005\u5922\u3000\u301c Ancient Temple"),
        QString::fromWCharArray(L"\u5e83\u6709\u5c04\u602a\u9ce5\u4e8b\u3000\u301c Till When?"),
        QString::fromWCharArray(L"\u30a2\u30eb\u30c6\u30a3\u30e1\u30c3\u30c8\u30c8\u30a5\u30eb\u30fc\u30b9"),
        QString::fromWCharArray(L"\u5e7d\u96c5\u306b\u54b2\u304b\u305b\u3001\u58a8\u67d3\u306e\u685c\u3000\u301c Border of Life"),
        QString::fromWCharArray(L"\u30dc\u30fc\u30c0\u30fc\u30aa\u30d6\u30e9\u30a4\u30d5"),
        QString::fromWCharArray(L"\u5996\u3005\u8dcb\u6248"),
        QString::fromWCharArray(L"\u5c11\u5973\u5e7b\u846c\u3000\u301c Necro-Fantasy"),
        QString::fromWCharArray(L"\u5996\u3005\u8dcb\u6248\u3000\u301c Who done it!"),
        QString::fromWCharArray(L"\u30cd\u30af\u30ed\u30d5\u30a1\u30f3\u30bf\u30b8\u30a2"),
        QString::fromWCharArray(L"\u6625\u98a8\u306e\u5922"),
        QString::fromWCharArray(L"\u3055\u304f\u3089\u3055\u304f\u3089\u3000\u301c Japanize Dream..."),
    };
    const int SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("Th07.dat");
    const QString BgmName("Thbgm.dat");

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

    void decompress(const QByteArray& compressed, QByteArray& decompressd)
    {
        QByteArray dict(0x2000, '\0');
        BitReader reader(compressed.data(), compressed.size());
        int cursor = 0;
        int dict_cursor = 1;
        char c;
        int addr, jump;
        do
        {
            if (reader.getBit())
            {
                c = reader.getChar();
                decompressd[cursor] = c;
                ++cursor;
                dict[dict_cursor] = c;
                dict_cursor = (dict_cursor + 1) & 0x1fff;
            }
            else
            {
                addr = reader.getBits(13);
                if (addr == 0)
                    return;
                jump = reader.getBits(4) + 3;
                for (int i = 0; i < jump; ++i)
                {
                    c = dict[addr];
                    addr = (addr + 1) & 0x1fff;
                    decompressd[cursor] = c;
                    ++cursor;
                    dict[dict_cursor] = c;
                    dict_cursor = (dict_cursor + 1) & 0x1fff;
                }
            }
        } while (true);
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
    dir = QDir(path);
    if (!dir.exists(FileName) || !dir.exists(BgmName))
        return false;

// PBG4 decompresser
    QFile file(dir.filePath(FileName));
    if (file.size() != 23828905)
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    uint max_file_count;
    uint header_pos;
    uint header_csize;
    uint header_dsize;
    {
        char buffer[16];
        if (file.read(buffer, 16) != 16)
            return false;
        if (getUInt32(buffer) != 0x34474250) //PBG4
            return false;
// Stage 1
        max_file_count = getUInt32(buffer + 4);
        header_pos = getUInt32(buffer + 8);
        header_dsize = getUInt32(buffer + 12);
        if (file.size() <= header_pos)
            return false;
        header_csize = file.size() - header_pos;
    }

    QByteArray thbgm_ddata;
    {
// Stage 2
        file.seek(header_pos);
        QByteArray cheader = file.read(header_csize);
        QByteArray dheader(header_dsize, '\0');
        decompress(cheader, dheader);
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
            while ( (c = dheader[cursor++]) )
                name += c;
            if (name != "thbgm.fmt")
            {
                cursor += 12;
                continue;
            }
            max_file_count = i + 2;
            thbgm_offset = getUInt32(dheader.data() + cursor); //offset
            cursor += 4;
            thbgm_dsize = getUInt64(dheader.data() + cursor); //size
            cursor += 8;
            while ( (c = dheader[cursor++]) )
                ;
            thbgm_csize = getUInt32(dheader.data() + cursor) - thbgm_offset;
            break;
        }
        file.seek(thbgm_offset);
        QByteArray thbgm_cdata(file.read(thbgm_csize));
        thbgm_ddata.resize(thbgm_dsize);
        decompress(thbgm_cdata, thbgm_ddata);
    }
// Stage3
    {
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
        info_list[SongDataSize - 1].size = 444516656 - info_list[SongDataSize - 1].offset;
    }

    QFile wav(dir.filePath(BgmName));
    if (wav.size() != 444516656)
        return false;
    if (!wav.open(QIODevice::ReadOnly))
        return false;
    return true;
}

MusicData Th07Loader::at(uint index)
{
    Q_ASSERT(0 <= index && index < SongDataSize);
    FileInfo info = info_list.at(index);

    return MusicData(
        ".wav",
        SongData[index],
        Title,
        info.size + 44,
        true,
        info.loopStart,
        info.loopEnd
    );
}

QByteArray Th07Loader::content(uint index)
{
    Q_ASSERT(0 <= index && index < SongDataSize);
    FileInfo info = info_list.at(index);
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

void Th07Loader::close()
{
    info_list.clear();
}
