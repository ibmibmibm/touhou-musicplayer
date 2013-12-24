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
#ifndef HELPERFUNCS_H
#define HELPERFUNCS_H
#include <QtEndian>

struct ThbgmData
{
    char name[16];
    quint32 offset;
    quint32 checksum;
    quint32 loopBegin;
    quint32 loopEnd;
    char header[16];
    quint32 PAD;
};

class BitReader
{
    public:
        BitReader(const QByteArray& _data) :
            pos(0),
            current(0),
            mask(0),
            bitOffset(0),
            data(_data)
        {}
        bool getBit()
        {
            bitOffset = (bitOffset + 1) & 0x7;
            if (!mask)
            {
                current = getNextByte();
                mask = 0x80;
            }
            bool ret = static_cast<bool>(current & mask);
            mask >>= 1;
            return ret;
        }
        uint getBits(uint len)
        {
            uint ret = 0;
            for (uint i = 0; i < len; ++i)
            {
                ret <<= 1;
                ret |= getBit();
            }
            return ret;
        }
        quint32 getUInt32()
        {
            return getBits((getBits(2) + 1) << 3);
        }
        uchar getChar()
        {
            if (!mask)
                return getNextByte();
            uchar ret = current << bitOffset;
            current = getNextByte();
            return ret | (current >> (8 - bitOffset));
        }
    protected:
        virtual quint8 getNextByte()
        {
            if (pos == data.size())
                return 0;
            return static_cast<quint8>(data[pos++]);
        }
        int pos;
        quint8 current;
        quint8 mask;
        quint8 bitOffset;
        const QByteArray data;
};

class ChecksumBitReader : public BitReader
{
    public:
        ChecksumBitReader(const QByteArray& _data) :
            BitReader(_data),
            sum(0)
        {}
        uint getSum() const { return sum; }
    protected:
        virtual quint8 getNextByte()
        {
            if (pos == data.size())
                return 0;
            sum += static_cast<quint8>(data[pos]);
            return static_cast<quint8>(data[pos++]);
        }
        uint sum;
};

QByteArray lzDecompress(const QByteArray& compressed, int decompressdSize = 0)
{
    QByteArray decompressd;
    decompressd.reserve(decompressdSize);
    QByteArray dict(0x2000, '\0');
    BitReader reader(compressed);
    int dict_cursor = 1;
    forever
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
    }
}

QByteArray lzDecompressChecksum(int& checksum, const QByteArray& compressed, int decompressdSize)
{
    QByteArray decompressd;
    decompressd.reserve(decompressdSize);
    QByteArray dict(0x2000, '\0');
    ChecksumBitReader reader(compressed);
    int dict_cursor = 1;
    forever
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
            {
                checksum = reader.getSum();
                return decompressd;
            }
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
    }
}

QByteArray lzDecompressDictSize(const QByteArray& compressed, size_t dictSize, int decompressdSize = 0)
{
    QByteArray decompressd;
    decompressd.reserve(decompressdSize);
    QByteArray dict(dictSize, '\0');
    BitReader reader(compressed);
    int dict_cursor = 1;
    forever
    {
        if (reader.getBit())
        {
            char c = reader.getChar();
            decompressd.append(c);
            dict[dict_cursor] = c;
            dict_cursor = (dict_cursor + 1) % dictSize;
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
                addr = (addr + 1) % dictSize;
                decompressd.append(c);
                dict[dict_cursor] = c;
                dict_cursor = (dict_cursor + 1) % dictSize;
            }
        }
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
// SoundForge sfl file parser
bool SFLParser(QIODevice& file, uint offset, uint size, uint& loopBegin, uint& loopEnd)
{
    uint parsestage = 0;
    uint file_size;
    quint32 chunk_size;
    quint32 int32;
    file.seek(offset);
    forever
    {
        quint32 marker;
        if (file.read(reinterpret_cast<char*>(&marker), sizeof(marker)) != sizeof(marker))
            return false;
        switch (marker)
        {
            case RIFF_MARKER:
                //qDebug() << "RIFF_MARKER";
                if (parsestage != 0)
                    return false;
                parsestage |= HAVE_RIFF;

                if (file.read(reinterpret_cast<char*>(&file_size), sizeof(file_size)) != sizeof(file_size))
                    return false;
                file_size = qFromLittleEndian(file_size) + 8;

                if (file_size > size)
                    return false;

                break;
            case WAVE_MARKER:
                //qDebug() << "WAVE_MARKER";
                if ((parsestage & HAVE_RIFF) != HAVE_RIFF)
                    return false;
                parsestage |= HAVE_WAVE;

                break;
            case fmt_MARKER:
                //qDebug() << "fmt_MARKER";

                if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
                    return false;
                parsestage |= HAVE_fmt;

                if (file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                file.seek(file.pos() + qFromLittleEndian(chunk_size));

                break;
            case data_MARKER:
                //qDebug() << "data_MARKER";

                if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
                    return false;

                parsestage |= HAVE_data;

                if (file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                file.seek(file.pos() + qFromLittleEndian(chunk_size));

                break;
            case cue_MARKER:
                //qDebug() << "cue_MARKER";

                if ((parsestage & HAVE_RIFF) != HAVE_RIFF)
                    return false;

                parsestage |= HAVE_cue;

                if (file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = file.pos() + qFromLittleEndian(chunk_size);

                if (file.read(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                if (file.read(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                if (file.read(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                loopBegin = qFromLittleEndian(int32);
                //qDebug() << "loopBegin" << loopBegin;
                file.seek(chunk_size);

                break;
            case LIST_MARKER:
                //qDebug() << "LIST_MARKER";

                if ((parsestage & HAVE_RIFF) != HAVE_RIFF)
                    return false;

                parsestage |= HAVE_LIST;

                if (file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = qFromLittleEndian(chunk_size);

                if ((parsestage & HAVE_ltxt) == HAVE_ltxt)
                    file.seek(file.pos() + chunk_size);

                break;
            case adtl_MARKER:
                //qDebug() << "adtl_MARKER";

                if ((parsestage & (HAVE_RIFF | HAVE_LIST)) != (HAVE_RIFF | HAVE_LIST))
                    return false;

                parsestage |= HAVE_adtl;

                break;
            case ltxt_MARKER:
                //qDebug() << "ltxt_MARKER";

                if ((parsestage & (HAVE_RIFF | HAVE_adtl)) != (HAVE_RIFF | HAVE_adtl))
                    return false;

                parsestage |= HAVE_ltxt;

                if (file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = file.pos() + qFromLittleEndian(chunk_size);

                if (file.read(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                if (file.read(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                loopEnd = qFromLittleEndian(int32);
                //qDebug() << "loopEnd" << loopEnd;

                file.seek(chunk_size);

                break;
            case labl_MARKER:
                //qDebug() << "labl_MARKER";

                if ((parsestage & (HAVE_RIFF | adtl_MARKER)) != (HAVE_RIFF | adtl_MARKER))
                    return false;

                parsestage |= HAVE_labl;

                if (file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                file.seek(file.pos() + qFromLittleEndian(chunk_size));

                break;
            default:
                //qDebug() << "other_MARKER" << QString::number(marker, 16);
                parsestage |= HAVE_other;
        }
        if (loopEnd != 0)
            break;
        if (file.pos() > offset + file_size)
            break;
    }
    loopEnd += loopBegin;
    return true;
}

#endif // HELPERFUNCS_H
