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
//#include <QtDebug>

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
    const int SongDataSize = sizeof(SongData) / sizeof(SongData[0]);
    const QString FileName = "th105b.dat";

    struct FileInfo
    {
        uint offset;
        uint size;
        QString name;
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

    void decodeData(char *data, const FileInfo &info)
    {
        char mask = (info.offset >> 1) | 0x23;

        for (uint i = 0; i < info.size; i++)
            data[i] ^= mask;
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
    QDir dir(path);
    if (!dir.exists(FileName))
        return false;

// dat file parser
    QFile file(dir.absoluteFilePath(FileName));
    if (file.size() != 67200123)
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    quint16 file_count;
    quint32 header_size;

    {
        char buffer[4];
        if (file.read(buffer, 2) != 2)
            return false;

        file_count = getUInt16(buffer);

        if (file.read(buffer, 4) != 4)
            return false;
        header_size = getUInt32(buffer);
    }

    char *header = new char[header_size];
    if (file.read(header, header_size) != header_size)
    {
        delete [] header;
        return false;
    }

    int mask[0x270];
    maskInit(mask, header_size);

    unsigned char c1 = 0xC5, c2 = 0x83;

    for (uint i = 0; i < header_size; i++)
    {
        if (i % 0x270 == 0) maskUpdate(mask);
        header[i] ^= maskGet(mask, i);
        header[i] ^= c1;
        c1 += c2;
        c2 += 0x53;
    }

    char *cursor = header;
    QList<FileInfo> info_list;

    for (int i = 0; i < file_count; ++i)
    {
        FileInfo info;
        info.offset = getUInt32(cursor);
        cursor += 4;

        info.size = getUInt32(cursor);
        cursor += 4;

        uint len = *cursor;
        info.name = QString::fromAscii(++cursor, len);
        cursor += len;

        if (info.name.endsWith(".ogg") || info.name.endsWith(".sfl"))
            info_list << info;
    }

    delete [] header;

    foreach(FileInfo info, info_list)
    {
        file.seek(info.offset);
        QByteArray arr(file.read(info.size));
        decodeData(arr.data(), info);
        data.insert(info.name, arr);
    }
    for (int i = 0; i < SongDataSize; ++i)
    {
        if (!data.contains(SongData[i][0] + ".ogg"))
        {
            data.clear();
            return false;
        }
    }
    return true;
}

MusicData Th105Loader::at(uint index)
{
    Q_ASSERT(0 <= index && index < SongDataSize);
    bool loop = false;
    uint loopStart = 0;
    uint loopEnd = 0;
    QString sfl = SongData[index][0] + ".sfl";

// SoundForge sfl file parser
    if (data.contains(sfl))
    {
        QByteArray cue = data.value(sfl);
        if (cue.size() > 16 && cue.startsWith("RIFF"))
        {
            uint loop_start = 0;
            uint loop_during = 0;
            int idx = 4;
            int data_size = getUInt32(cue.data() + idx) + idx;
            idx += 4;
            char *data_title = cue.data() + idx;
            idx += 4;
            if (qstrncmp(data_title, "SFPL", 4) == 0)
            {
                while (idx < data_size)
                {
                    char *section_title = cue.data() + idx;
                    idx += 4;
                    int section_size = getUInt32(cue.data() + idx);
                    idx += 4;
                    int section_end = section_size + idx;
                    if (qstrncmp(section_title, "cue ", 4) == 0)
                    {
                        Q_ASSERT(getUInt32(cue.data() + idx) == 1);
                        idx += 4;
                        Q_ASSERT(getUInt32(cue.data() + idx) == 1);
                        idx += 4;
                        loop_start = getUInt32(cue.data() + idx);
                        idx += 4;
                        Q_ASSERT(qstrncmp(cue.data() + idx, "data", 4) == 0);
                        idx += 4;
                        Q_ASSERT(getUInt32(cue.data() + idx) == 0);
                        idx += 4;
                        Q_ASSERT(getUInt32(cue.data() + idx) == 0);
                        idx += 4;
                        Q_ASSERT(getUInt32(cue.data() + idx) == loop_start);
                        idx += 4;
                        Q_ASSERT(idx == section_end);
                    }
                    else if (qstrncmp(section_title, "LIST", 4) == 0)
                    {
                        /*char *list_type = cue.data() + idx;*/
                        idx += 4;
                        /*Q_ASSERT(qstrncmp(list_type, "adtl", 4) == 0);*/
                        while (idx < section_end)
                        {
                            char *frag_title = cue.data() + idx;
                            idx += 4;
                            int frag_size = getUInt32(cue.data() + idx);
                            idx += 4;
                            int frag_end = frag_size + idx;
                            if (qstrncmp(frag_title, "ltxt", 4) == 0)
                            {
                                Q_ASSERT(getUInt32(cue.data() + idx) == 1);
                                idx += 4;
                                loop_during = getUInt32(cue.data() + idx);
                                idx += 4;
                                Q_ASSERT(qstrncmp(cue.data() + idx, "rgn ", 4) == 0);
                                idx += 4;
                                Q_ASSERT(getUInt32(cue.data() + idx) == 0);
                                idx += 4;
                                Q_ASSERT(getUInt32(cue.data() + idx) == 0);
                                idx += 4;
                                Q_ASSERT(idx == frag_end);
                            }
                            else if (qstrncmp(frag_title, "labl", 4) == 0)
                            {
                                Q_ASSERT(getUInt32(cue.data() + idx) == 1);
                                idx += 4;
                                //qDebug("%s", cue.data() + idx);
                                idx = frag_end;
                            }
                        }
                    }
                    else if (qstrncmp(section_title, "SFPI", 4) == 0)
                    {
                        //qDebug("%s", cue.data() + idx);
                        idx = section_end;
                    }
                }
            }
            //qDebug("start=%u during=%u", loop_start, loop_during);
            loop = true;
            loopStart = loop_start * 0.02267573696145124716553287982 + 0.5; // 1000/44100
            loopEnd = (loop_start + loop_during) * 0.02267573696145124716553287982 + 0.5; // 1000/44100
        }
    }
    return MusicData(
        data.value(SongData[index][0] + ".ogg"),
        ".ogg",
        SongData[index][1],
        Title,
        loop,
        loopStart,
        loopEnd
    );
}

void Th105Loader::close()
{
    data.clear();
}
