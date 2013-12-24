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

#include "th105loader.h"
#include "helperfuncs.h"

Q_EXPORT_PLUGIN2("Th105Loader", Th105Loader)

namespace {
    const QString Title = QString::fromWCharArray(L"\u6771\u65b9\u7dcb\u60f3\u5929\u3000\u301c Scarlet Weather Rhapsody");
    const QString SongData[][2] = {
        {"data/bgm/op", QString::fromWCharArray(L"\u7dcb\u60f3\u5929")},
        {"data/bgm/sr", QString::fromWCharArray(L"\u6771\u65b9\u7dcb\u60f3\u5929")},
        {"data/bgm/ta00", QString::fromWCharArray(L"\u65e5\u5e38\u5750\u81e5")},
        {"data/bgm/ta01", QString::fromWCharArray(L"\u7532\u8ad6\u4e59\u99c1")},
        {"data/bgm/ta02", QString::fromWCharArray(L"\u96f2\u5916\u84bc\u5929")},
        {"data/bgm/ta03", QString::fromWCharArray(L"\u98a8\u5149\u660e\u5a9a")},
        {"data/bgm/ta04", QString::fromWCharArray(L"\u51b7\u541f\u9591\u9154")},
        {"data/bgm/ta05", QString::fromWCharArray(L"\u4ee5\u9b5a\u99c6\u8805")},
        {"data/bgm/ta06", QString::fromWCharArray(L"\u66ae\u8272\u84bc\u7136")},
        {"data/bgm/ta07", QString::fromWCharArray(L"\u5929\u8863\u7121\u7e2b")},
        {"data/bgm/ta08", QString::fromWCharArray(L"\u653e\u7e26\u4e0d\u7f88")},
        {"data/bgm/st00", QString::fromWCharArray(L"\u5730\u306e\u8272\u306f\u9ec4\u8272")},
        {"data/bgm/st01", QString::fromWCharArray(L"\u9999\u308b\u6a39\u8449\u82b1")},
        {"data/bgm/st02", QString::fromWCharArray(L"\u8e0a\u308b\u6c34\u98db\u6cab")},
        {"data/bgm/st03", QString::fromWCharArray(L"\u5632\u308a\u306e\u904a\u622f")},
        {"data/bgm/st04", QString::fromWCharArray(L"\u9ed2\u3044\u6d77\u306b\u7d05\u304f \uff5e Legendary Fish")},
        {"data/bgm/st05", QString::fromWCharArray(L"\u6709\u9802\u5929\u5909 \uff5e Wonderful Heaven")},
        {"data/bgm/st06", QString::fromWCharArray(L"\u5e7c\u5fc3\u5730\u306e\u6709\u9802\u5929")},
        {"data/bgm/st10", QString::fromWCharArray(L"\u6771\u65b9\u5996\u604b\u8ac7")},
        {"data/bgm/st11", QString::fromWCharArray(L"\u661f\u306e\u5668 \uff5e Casket of Star")},
        {"data/bgm/st12", QString::fromWCharArray(L"\u30d5\u30e9\u30ef\u30ea\u30f3\u30b0\u30ca\u30a4\u30c8")},
        {"data/bgm/st13", QString::fromWCharArray(L"\u30d6\u30af\u30ec\u30b7\u30e5\u30c6\u30a3\u306e\u4eba\u5f62\u5e2b")},
        {"data/bgm/st14", QString::fromWCharArray(L"\u5e83\u6709\u5c04\u602a\u9ce5\u4e8b \uff5e Till When\uff1f")},
        {"data/bgm/st15", QString::fromWCharArray(L"\u30e9\u30af\u30c8\u30ac\u30fc\u30eb \uff5e \u5c11\u5973\u5bc6\u5ba4")},
        {"data/bgm/st16", QString::fromWCharArray(L"\u5e7d\u96c5\u306b\u54b2\u304b\u305b\u3001\u58a8\u67d3\u306e\u685c \uff5e Border of Life")},
        {"data/bgm/st17", QString::fromWCharArray(L"\u4ea1\u304d\u738b\u5973\u306e\u70ba\u306e\u30bb\u30d7\u30c6\u30c3\u30c8")},
        {"data/bgm/st18", QString::fromWCharArray(L"\u591c\u304c\u964d\u308a\u3066\u304f\u308b")},
        {"data/bgm/st19", QString::fromWCharArray(L"\u7815\u6708")},
        {"data/bgm/st20", QString::fromWCharArray(L"\u72c2\u6c17\u306e\u77b3 \uff5e Invisible Full Moon")},
        {"data/bgm/st21", QString::fromWCharArray(L"\u98a8\u795e\u5c11\u5973")},
        {"data/bgm/st22", QString::fromWCharArray(L"\u5f7c\u5cb8\u5e30\u822a \uff5e Riverside View")},
    };
    const uint SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName("th105b.dat");

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

const QString& Th105Loader::title() const
{
    return Title;
}

uint Th105Loader::size() const
{
    return SongDataSize;
}

bool Th105Loader::open(const QString &path)
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
        char buffer[4];
        if (file.read(buffer, 2) != 2)
            return false;

        file_count = getUInt16(buffer);

        if (file.read(buffer, 4) != 4)
            return false;
        header_size = getUInt32(buffer);
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
        int cursor = 0;
        for (int i = 0; i < file_count; ++i)
        {
            FileInfo info;
            info.loopBegin = info.loopEnd = 0;
            info.offset = getUInt32(header.data() + cursor);
            cursor += 4;
            info.size = getUInt32(header.data() + cursor);
            cursor += 4;
            uint len = getUInt8(header.data() + cursor);
            ++cursor;
            QString name = QString::fromAscii(header.data() + cursor, len);
            cursor += len;

            if (name.endsWith(".ogg") || name.endsWith(".sfl"))
            {
                info_hash.insert(name, info);
            }
        }
    }

    for (uint i = 0; i < SongDataSize; ++i)
    {
        QString ogg = SongData[i][0] + ".ogg";
        if (!info_hash.contains(ogg))
            return false;
        FileInfo info = info_hash.value(ogg);

// SoundForge sfl file parser
        QString sfl = SongData[i][0] + ".sfl";
        if (info_hash.contains(sfl))
        {
            FileInfo cueinfo = info_hash.value(sfl);

            file.seek(cueinfo.offset);
            QByteArray cue(file.read(cueinfo.size));
            decode(cue.data(), cueinfo);

            if (cue.size() > 16 && cue.startsWith("RIFF"))
            {
                int cursor = 4;
                int data_size = getUInt32(cue.data() + cursor) + cursor;
                cursor += 4;
                char *data_title = cue.data() + cursor;
                cursor += 4;
                if (qstrncmp(data_title, "SFPL", 4) == 0)
                {
                    while (cursor < data_size)
                    {
                        char *section_title = cue.data() + cursor;
                        cursor += 4;
                        int section_size = getUInt32(cue.data() + cursor);
                        cursor += 4;
                        int section_end = section_size + cursor;
                        if (qstrncmp(section_title, "cue ", 4) == 0)
                        {
                            Q_ASSERT(getUInt32(cue.data() + cursor) == 1);
                            cursor += 4;
                            Q_ASSERT(getUInt32(cue.data() + cursor) == 1);
                            cursor += 4;
                            info.loopBegin = getUInt32(cue.data() + cursor);
                            cursor += 4;
                            Q_ASSERT(qstrncmp(cue.data() + cursor, "data", 4) == 0);
                            cursor += 4;
                            Q_ASSERT(getUInt32(cue.data() + cursor) == 0);
                            cursor += 4;
                            Q_ASSERT(getUInt32(cue.data() + cursor) == 0);
                            cursor += 4;
                            Q_ASSERT(getUInt32(cue.data() + cursor) == info.loopBegin);
                            cursor += 4;
                            Q_ASSERT(cursor == section_end);
                        }
                        else if (qstrncmp(section_title, "LIST", 4) == 0)
                        {
                            Q_ASSERT(qstrncmp(cue.data(), "RIFF", 4) == 0);
                            cursor += 4;
                            while (cursor < section_end)
                            {
                                char *frag_title = cue.data() + cursor;
                                cursor += 4;
                                int frag_size = getUInt32(cue.data() + cursor);
                                cursor += 4;
                                int frag_end = frag_size + cursor;
                                if (qstrncmp(frag_title, "ltxt", 4) == 0)
                                {
                                    Q_ASSERT(getUInt32(cue.data() + cursor) == 1);
                                    cursor += 4;
                                    info.loopEnd = getUInt32(cue.data() + cursor);
                                    cursor += 4;
                                    Q_ASSERT(qstrncmp(cue.data() + cursor, "rgn ", 4) == 0);
                                    cursor += 4;
                                    Q_ASSERT(getUInt32(cue.data() + cursor) == 0);
                                    cursor += 4;
                                    Q_ASSERT(getUInt32(cue.data() + cursor) == 0);
                                    cursor += 4;
                                    Q_ASSERT(cursor == frag_end);
                                }
                                else if (qstrncmp(frag_title, "labl", 4) == 0)
                                {
                                    Q_ASSERT(getUInt32(cue.data() + cursor) == 1);
                                    cursor += 4;
                                    //qDebug("%s", cue.data() + cursor);
                                    cursor = frag_end;
                                }
                            }
                        }
                        else if (qstrncmp(section_title, "SFPI", 4) == 0)
                        {
                            //qDebug("%s", cue.data() + cursor);
                            cursor = section_end;
                        }
                    }
                }
                //qDebug("start=%u during=%u", loop_begin, loop_during);
                info.loopEnd += info.loopBegin;
            }
        }
        else // for data/bgm/sr.ogg
        {
            info.loopEnd = 3724704 - 44100;
        }
        info_list << info;
    }
    return true;
}

MusicData Th105Loader::at(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list[index];
    ArchiveMusicData archiveMusicData(dir.absoluteFilePath(FileName), info.offset, info.offset + info.size,
        decoder, decoder, reinterpret_cast<void*>(static_cast<quintptr>(((info.offset >> 1) & 0xff) | 0x23)));
    //qDebug() << info.name << Title;

    return MusicData(
        SongData[index][0] + ".ogg",
        SongData[index][1],
        Title,
        ".ogg",
        info.size,
        true,
        info.loopBegin,
        info.loopEnd,
        &archiveMusicData
    );
}

QByteArray Th105Loader::content(uint index)
{
    Q_ASSERT(index < SongDataSize);
    FileInfo info = info_list[index];

    QFile file(dir.absoluteFilePath(FileName));
    file.open(QIODevice::ReadOnly);

    file.seek(info.offset);
    QByteArray ogg(file.read(info.size));
    decode(ogg.data(), info);

    return ogg;
}

void Th105Loader::close()
{
    info_list.clear();
}
