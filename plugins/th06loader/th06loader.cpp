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

    class BitReader
    {
        public:
            BitReader(const char*const _data, const int _size) :
                pos(0),
                sum(0),
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
                    sum += static_cast<unsigned char>(data[pos]);
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
            uint getSum() const { return sum; }
        private:
            uint pos;
            uint sum;
            quint8 bitPos;
            const uint size;
            const char*const data;
    };

    int decompress(const QByteArray& compressed, QByteArray& decompressd)
    {
        QByteArray dict(0x2000, '\0');
        BitReader reader(compressed.data(), compressed.size());
        int cursor = 0;
        int dict_cursor = 1;
        char c;
        int addr, jump;
        forever
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
                    break;
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
        }
        return reader.getSum();
    }
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
    dir = QDir(path);
    if (!dir.exists(FileName))
        return false;

// PBG3 decompresser
    QFile file(dir.filePath(FileName));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    uint max_file_count;
    uint header_pos;
    uint header_size;
    {
        char buffer[9];
        if (file.read(buffer, 4) != 4)
            return false;
        if (getUInt32(buffer) != 0x33474250) //PBG3
            return false;
// Stage 1
        if (file.read(buffer, 9) != 9)
            return false;
        BitReader reader(buffer, 9);
        max_file_count = reader.getUInt32();
        header_pos = reader.getUInt32();
        if (file.size() <= header_pos)
            return false;
        header_size = file.size() - header_pos;
    }

    QList<FileInfo> info_list;
    {
// Stage 2
        file.seek(header_pos);
        QByteArray header = file.read(header_size);
        BitReader reader(header.data(), header_size);
        for (uint i = 0; i < max_file_count; ++i)
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
            info_list << info;
        }
        // dummy info
        FileInfo info;
        info.offset = header_pos;
        info_list << info;
        for (int i = 0; i < info_list.size() - 1; ++i)
        {
            FileInfo info = info_list.at(i);
            if (!info.name.endsWith(".pos"))
                continue;
            uint csize = info_list.at(i + 1).offset - info.offset;
            file.seek(info.offset);
            QByteArray cdata(file.read(csize));
            QByteArray ddata(info.size, '\0');
            int checksum = decompress(cdata, ddata);
            if (checksum == info.checksum)
                data.insert(info.name, ddata);
            else
                qDebug() << QString("file %1: checksum error!").arg(info.name);
        }
    }

    for (uint i = 0; i < SongDataSize; ++i)
    {
        if (!dir.exists(WavName.arg(i + 1, 2, 10, QLatin1Char('0'))))
        {
            data.clear();
            return false;
        }
    }
    return true;
}

MusicData Th06Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    QString pos = PosName.arg(index + 1, 2, 10, QLatin1Char('0'));
    Q_ASSERT(data.contains(pos));
    const char *d = data.value(pos).data();
    uint loopBegin = getUInt32(d);
    uint loopEnd = getUInt32(d + 4);

    QFile file(dir.absoluteFilePath(WavName.arg(index + 1, 2, 10, QLatin1Char('0'))));

    return MusicData(
        file.fileName(),
        SongData[index],
        Title,
        ".wav",
        file.size(),
        true,
        loopBegin,
        loopEnd
    );
}

QByteArray Th06Loader::content(uint index)
{
    Q_ASSERT(index < SongDataSize);
    QFile file(dir.absoluteFilePath(WavName.arg(index + 1, 2, 10, QLatin1Char('0'))));
    file.open(QIODevice::ReadOnly);

    return file.readAll();
}

void Th06Loader::close()
{
    data.clear();
}
