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
#include <QtEndian>
#include "musicfile_wav.h"

template <typename T>
inline T FromEndian(MusicFile_Wav::_Endian endian, const T &value)
{
    if (endian == MusicFile_Wav::_LittleEndian)
        return qFromLittleEndian(value);
    return qFromBigEndian(value);
}
template <typename T, typename U>
bool MusicFile_Wav::_getFieldFromFile(U &data)
{
    T tempData;
    if (sizeof(tempData) != MusicFile::_readData(reinterpret_cast<char*>(&tempData), sizeof(tempData)))
        return false;
    data = FromEndian(_endian, tempData);
    return true;
}

bool MusicFile_Wav::_parseHeader()
{
    MusicFile::seek(0);

    /* port from libsndfile */
    enum
    {
        HAVE_RIFF  = 0x01,
        HAVE_WAVE  = 0x02,
        HAVE_fmt   = 0x04,
        HAVE_fact  = 0x08,
        HAVE_PEAK  = 0x10,
        HAVE_data  = 0x20,
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
        RIFX_MARKER = MAKE_MARKER('R','I','F','X'),
        WAVE_MARKER = MAKE_MARKER('W','A','V','E'),
        fmt_MARKER  = MAKE_MARKER('f','m','t',' '),
        data_MARKER = MAKE_MARKER('d','a','t','a'),
        LIST_MARKER = MAKE_MARKER('L','I','S','T'),
        cue_MARKER  = MAKE_MARKER('c','u','e',' '),
        iwep_MARKER = MAKE_MARKER('i','w','e','p'),
        iwem_MARKER = MAKE_MARKER('i','w','e','m'),
        acid_MARKER = MAKE_MARKER('a','c','i','d'),
        inst_MARKER = MAKE_MARKER('i','n','s','t'),
        Appl_MARKER = MAKE_MARKER('A','p','p','l'),
    };

#undef MAKE_MARKER

    enum
    {
        WAVE_FORMAT_PCM = 0x0001,
        WAVE_FORMAT_IEEE_FLOAT = 0x0003,
    };

    bool done = false;
    int parsestage = 0;
    quint32 file_size;
    quint32 chunk_size;
    quint16 _bitwidth;
    while (!done)
    {
        quint32 marker;
        if (MusicFile::_readData(reinterpret_cast<char*>(&marker), sizeof(marker)) != sizeof(marker))
            return false;
        switch (marker)
        {
            case RIFF_MARKER:
            case RIFX_MARKER:
                {
                    if (parsestage != 0)
                        return false;

                    parsestage |= HAVE_RIFF;

                    if (marker == RIFX_MARKER)
                        _endian = _BigEndian;
                    if (!_getFieldFromFile<quint32>(file_size))
                        return false;
                    file_size += 8;
                    if (file_size > MusicFile::size())
                        return false;
                }
                break;
            case WAVE_MARKER:
                {
                    if ((parsestage & HAVE_RIFF) != HAVE_RIFF)
                        return false;
                    parsestage |= HAVE_WAVE;
                }
                break;
            case fmt_MARKER:
                {
                    if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
                        return false;

                    /* If this file has a SECOND fmt chunk, I don't want to know about it. */
                    if (parsestage & HAVE_fmt)
                        break;

                    parsestage |= HAVE_fmt;

                    if (!_getFieldFromFile<quint32>(chunk_size))
                        return false;
                    if (chunk_size < 16)
                        return false;
                    if (!_getFieldFromFile<quint16>(_format))
                        return false;
                    if (_format != WAVE_FORMAT_PCM)
                        return false;
                    if (!_getFieldFromFile<quint16>(_channels))
                        return false;
                    if (!_getFieldFromFile<quint32>(_samplerate))
                        return false;
                    if (!_getFieldFromFile<quint32>(_bytespersec))
                        return false;
                    if (!_getFieldFromFile<quint16>(_blockalign))
                        return false;
                    if (!_getFieldFromFile<quint16>(_bitwidth))
                        return false;
                    _bytewidth = (_bitwidth + 7) >> 3;
                    _blockwidth = _bytewidth * _channels;
                }
                break;
            case data_MARKER:
                {
                    if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
                        return false;

                    parsestage |= HAVE_data;

                    if (!_getFieldFromFile<quint32>(chunk_size))
                        return false;

                    _dataBegin = MusicFile::_pos();
                    _dataSize = chunk_size;
                    if (chunk_size == 0 && file_size == 16 && MusicFile::size() > 44)
                    {
                        /* Looks like a WAV file which wasn't closed properly. Fixing it. */
                        _dataSize = chunk_size = MusicFile::size() - _dataBegin;
                    }
                    if (_dataSize > MusicFile::size() - _dataBegin)
                    {
                        _dataSize = MusicFile::size() - _dataBegin;
                    }
                    _dataEnd = qMin(_dataSize + _dataBegin, MusicFile::size());
                    MusicFile::seek(_dataEnd);
                }
                break;
            case LIST_MARKER:
            case cue_MARKER:
            case iwep_MARKER:
            case iwem_MARKER:
            case acid_MARKER:
            case inst_MARKER:
            case Appl_MARKER:
                {
                    parsestage |= HAVE_other;

                    if (!_getFieldFromFile<quint32>(chunk_size))
                        return false;

                    quint32 nextPos = MusicFile::_pos() + chunk_size;
                    if (nextPos > MusicFile::size())
                        return false;
                    MusicFile::seek(nextPos);
                }
                break;
            default:
                //qDebug() << "other_MARKER" << QString::number(marker, 16);
                return false;
        }
        if (MusicFile::_pos() >= MusicFile::size() - 4)
            break;
    }
    //qDebug() << _dataBegin;
    if (!_dataBegin)
        return false;
    return true;
}

void MusicFile_Wav::_removeLeadingZeros()
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(pos() == 0);
    qint16* samples = new qint16[channels()];
    while (MusicFile::_readData(reinterpret_cast<char*>(samples), _blockwidth) == _blockwidth)
    {
        bool isZeros = true;
        for (uint i = 0; i < _channels; ++i)
        {
            if (samples[i] != 0)
            {
                isZeros = false;
                break;
            }
        }
        if (!isZeros)
            break;
        _dataBegin += _blockwidth;
        _dataSize -= _blockwidth;
        --_loopBegin;
        --_loopEnd;
    }
    delete samples;
    seek(0);
}

void MusicFile_Wav::_initializeAsRawData()
{
    //qDebug() << Q_FUNC_INFO;
    _dataBegin = 0;
    _dataSize = _dataEnd = MusicFile::size();
    _format = 0;
    _channels = 2;
    _samplerate = 44100;
    _bytespersec = 44100 * 2 * 2;
    _blockalign = 4;
    _bytewidth = 2;
    _blockwidth = _bytewidth * _channels;
}

MusicFile_Wav::MusicFile_Wav(const MusicData& fileDescription) :
    MusicFile(fileDescription),
    _endian(_LittleEndian),
    _dataBegin(0),
    _dataSize(0),
    _dataEnd(0)
{
}

bool MusicFile_Wav::open(OpenMode mode)
{
    if (!MusicFile::open(mode))
        return false;
    Q_ASSERT(!(mode & QIODevice::ReadOnly) || !(mode & QIODevice::Text));
    Q_ASSERT(!(mode & QIODevice::Text));
    Q_ASSERT(!(mode & QIODevice::Append));
    if (!_parseHeader())
        _initializeAsRawData();
    seek(0);

    _removeLeadingZeros();
    return true;
}

qint64 MusicFile_Wav::size() const
{
    return _dataSize;
}

bool MusicFile_Wav::seek(qint64 pos)
{
    return QIODevice::seek(pos) && MusicFile::_seek(pos + _dataBegin);
}

bool MusicFile_Wav::reset()
{
    return MusicFile::reset() && seek(0);
}

qint64 MusicFile_Wav::readData(char* data, qint64 maxSize)
{
    if (MusicFile::_pos() < _dataBegin)
        MusicFile::seek(_dataBegin);
    if (MusicFile::_pos() > _dataEnd)
        MusicFile::seek(_dataEnd);
    qint64 realMaxSize = qMin(maxSize, _dataEnd - MusicFile::_pos());
    return MusicFile::readData(data, realMaxSize);
}

