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
#include <QFile>
#include <QHash>
#include <QBuffer>

#include "th123loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th123Loader", Th123Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u975e\u60f3\u5929\u5247 \u301c\u8d85\u5f29\u7d1a\u30ae\u30cb\u30e7\u30eb\u306e\u8b0e\u3092\u8ffd\u3048\u301c");
    const QString SongData[][3] = {
        {"data/bgm/op2", QString::fromWCharArray(L"君はあの影を見たか"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/sr2", QString::fromWCharArray(L"悠久の蒸気機関"), QString::fromWCharArray(L"Unknow")},
        //{"data/bgm/room", QString::fromWCharArray(L"room"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/select", QString::fromWCharArray(L"萃夢想"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st30", QString::fromWCharArray(L"信仰は儚き人間の為に"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st31", QString::fromWCharArray(L"おてんば恋娘"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st32", QString::fromWCharArray(L"上海紅茶館　～ Chinese Tea"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st33", QString::fromWCharArray(L"霊知の太陽信仰　～ Nuclear Fusion"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st34", QString::fromWCharArray(L"明日ハレの日、ケの昨日"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st35", QString::fromWCharArray(L"アンノウンＸ　～ Unfound Adventure"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st36", QString::fromWCharArray(L"空に浮かぶ物体Ｘ"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st40", QString::fromWCharArray(L"二色蓮花蝶　～ Ancients"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st41", QString::fromWCharArray(L"恋色マジック"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st42", QString::fromWCharArray(L"the Grimoire of Alice"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st43", QString::fromWCharArray(L"ヴワル魔法図書館"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/st99", QString::fromWCharArray(L"東方妖恋談"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta00", QString::fromWCharArray(L"日常坐臥"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta01", QString::fromWCharArray(L"甲論乙駁"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta03", QString::fromWCharArray(L"風光明媚"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta04", QString::fromWCharArray(L"冷吟閑酔"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta05", QString::fromWCharArray(L"以魚駆蠅"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta06", QString::fromWCharArray(L"暮色蒼然"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta20", QString::fromWCharArray(L"伝説の巨神"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta21", QString::fromWCharArray(L"ぼくらの非想天則"), QString::fromWCharArray(L"Unknow")},
        {"data/bgm/ta22", QString::fromWCharArray(L"人形のある風景"), QString::fromWCharArray(L"Unknow")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th123b.dat");

    struct FileDescription
    {
        quint32 offset;
        quint32 size;
        quint8 len;
    };

    void maskInit(int mask[0x270], int s)
    {
        mask[0] = s + 6;

        for (int i = 1; i < 0x270; ++i)
        {
            uint m = mask[i - 1];
            m >>= 0x1E;
            m ^= mask[i-1];
            m *= 0x6C078965;
            m += i;
            mask[i] = m;
        }
    }

    void maskUpdate(int mask[0x270])
    {
        for (int i = 0; i < 0xE3; ++i)
        {
            uint m = mask[i + 1];
            m ^= mask[i];
            m &= 0x7FFFFFFF;
            m ^= mask[i];
            int p = m;
            m >>= 1;
            p &= 1;
            m ^= ((p) ? 0x9908B0DF : 0);
            m ^= mask[0x18C+i+1];
            mask[i] = m;
        }

        for (int i = 0xE3; i < 0x26F; i++)
        {
            uint m = mask[i];
            m ^= mask[i+1];
            m &= 0x7FFFFFFF;
            m ^= mask[i];
            int p = m;
            p &= 1;
            p = ((p) ? 0x9908B0DF : 0);
            p ^= mask[i-0xE3];
            m >>= 1;
            p ^= m;
            mask[i] = p;
        }

        int p = mask[0x26F];
        uint m = mask[0];
        m ^= p;
        m &= 0x7FFFFFFF;
        m ^= p;
        p = m;
        m >>= 1;
        p &= 1;
        m ^= ((p) ? 0x9908B0DF : 0);
        m ^= mask[0x18C];
        mask[0x26F] = m;
    }

    char maskGet(int mask[0x270], int n)
    {
        int m = mask[n % 0x270];
        uint p = m;
        p >>= 0xB;
        m ^= p;
        uint s = m;
        s &= 0xFF3A58AD;
        s <<= 7;
        m ^= s;
        p = m;
        p &= 0xFFFFDF8C;
        p <<= 0xF;
        m ^= p;
        s = m;
        s >>= 0x12;
        s ^= m;

        return s;
    }

    void decode(char* data, const FileInfo &info)
    {
        char mask = (info.offset >> 1) | 0x23;

        for (uint i = 0; i < info.size; i++)
            data[i] ^= mask;
    }

    char decoder(void*p, char c)
    {
        return c ^ static_cast<char>(reinterpret_cast<quintptr>(p));
    }
}

const QString& Th123Loader::title() const
{
    return Title;
}

uint Th123Loader::size() const
{
    return SongDataSize;
}

bool Th123Loader::open(const QString &path)
{
    dir = QDir(path);
    if (!dir.exists(FileName))
        return false;

// dat file parser
    QFile file(dir.absoluteFilePath(FileName));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    quint16 file_count;
    qint32 header_size;
    {
        if (file.read(reinterpret_cast<char*>(&file_count), 2) != 2)
            return false;

        file_count = qFromLittleEndian(file_count);

        if (file.read(reinterpret_cast<char*>(&header_size), 4) != 4)
            return false;
        header_size = qFromLittleEndian(header_size);
    }

    QHash<QString, FileInfo> info_hash;
    {
        QByteArray header = file.read(header_size);
        if (header.size() != header_size)
            return false;

        // read header
        {
            int mask[0x270];
            maskInit(mask, header_size);

            unsigned char c1 = 0xC5, c2 = 0x83;

            for (int i = 0; i < header_size; i++)
            {
                if (i % 0x270 == 0) maskUpdate(mask);
                header.data()[i] ^= maskGet(mask, i);
                header.data()[i] ^= c1;
                c1 += c2;
                c2 += 0x53;
            }
        }

        // parse header
        char* cursor = header.data();
        for (int i = 0; i < file_count; ++i)
        {
            FileInfo info;
            info.loopBegin = info.loopEnd = 0;
            FileDescription* fileDescription = reinterpret_cast<FileDescription*>(cursor);
            cursor += 9;
            info.offset = qFromLittleEndian(fileDescription->offset);
            info.size = qFromLittleEndian(fileDescription->size);
            uint len = qFromLittleEndian(fileDescription->len);
            QString name = QString::fromAscii(cursor, len);
            cursor += len;

            if (name.endsWith(".ogg") || name.endsWith(".sfl"))
            {
                info_hash.insert(name, info);
            }
        }
    }

    for (uint i = 0; i < SongDataSize; ++i)
    {
        QString ogg(SongData[i][0] + ".ogg");
        if (!info_hash.contains(ogg))
            return false;
        FileInfo info = info_hash.value(ogg);

        QString sfl(SongData[i][0] + ".sfl");

        if (info_hash.contains(sfl))
        {
            FileInfo cueinfo = info_hash.value(sfl);

            file.seek(cueinfo.offset);
            QByteArray cue(file.read(cueinfo.size));
            decode(cue.data(), cueinfo);

            QBuffer cuefile(&cue);
            cuefile.open(QIODevice::ReadOnly);
            if (!SFLParser(cuefile, 0, cueinfo.size, info.loopBegin, info.loopEnd))
                return false;
        }
        else // for sr2.ogg
        {
            info.loopEnd = 4412892 - 44100*7;
        }

        info_list << info;
    }
    return true;
}

MusicData Th123Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list[index];
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(FileName), info.offset, info.offset + info.size,
        decoder, decoder, reinterpret_cast<void*>(static_cast<quintptr>(((info.offset >> 1) & 0xff) | 0x23)));

    return MusicData(
        SongData[index][0] + ".ogg",
        SongData[index][1],
        SongData[index][2],
        Title,
        index + 1,
        SongDataSize,
        ".ogg",
        info.size,
        true,
        info.loopBegin,
        info.loopEnd,
        &archiveMusicData
    );
}

void Th123Loader::close()
{
    info_list.clear();
}
