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
inline T FromEndian(MusicFile_Wav::_Endian endian, T value)
{
    if (endian == MusicFile_Wav::_LittleEndian)
        return qFromLittleEndian(value);
    return qFromBigEndian(value);
}

bool MusicFile_Wav::_parseHeader()
{
    //qDebug() << Q_FUNC_INFO;

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
    quint32 int32;
    quint16 int16;
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
                //qDebug() << "RIFF_MARKER";
                if (parsestage != 0)
                    return false;

                parsestage |= HAVE_RIFF;

                if (marker == RIFX_MARKER)
                    _endian = _BigEndian;

                if (MusicFile::_readData(reinterpret_cast<char*>(&file_size), sizeof(file_size)) != sizeof(file_size))
                    return false;
                file_size = FromEndian(_endian, file_size) + 8;

                if (file_size > MusicFile::size())
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

                /* If this file has a SECOND fmt chunk, I don't want to know about it. */
                if (parsestage & HAVE_fmt)
                    break;

                parsestage |= HAVE_fmt;

                if (MusicFile::_readData(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = FromEndian(_endian, chunk_size);

                //qDebug() << "chunk_size" << chunk_size;
                if (chunk_size < 16)
                    return false;

                if (MusicFile::_readData(reinterpret_cast<char*>(&int16), sizeof(int16)) != sizeof(int16))
                    return false;
                _format = FromEndian(_endian, int16);
                //qDebug() << "_format" << _format;

                if (_format != WAVE_FORMAT_PCM)
                    return false;

                if (MusicFile::_readData(reinterpret_cast<char*>(&int16), sizeof(int16)) != sizeof(int16))
                    return false;
                _channels = FromEndian(_endian, int16);
                //qDebug() << "_channels" << _channels;

                if (MusicFile::_readData(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                _samplerate = FromEndian(_endian, int32);
                //qDebug() << "_samplerate" << _samplerate;

                if (MusicFile::_readData(reinterpret_cast<char*>(&int32), sizeof(int32)) != sizeof(int32))
                    return false;
                _bytespersec = FromEndian(_endian, int32);
                //qDebug() << "_bytespersec" << _bytespersec;

                if (MusicFile::_readData(reinterpret_cast<char*>(&int16), sizeof(int16)) != sizeof(int16))
                    return false;
                _blockalign = FromEndian(_endian, int16);
                //qDebug() << "_blockalign" << _blockalign;

                if (MusicFile::_readData(reinterpret_cast<char*>(&int16), sizeof(int16)) != sizeof(int16))
                    return false;
                _bitwidth = FromEndian(_endian, int16);
                _bytewidth = (_bitwidth + 7) >> 3;
                //qDebug() << "_bytewidth" << _bytewidth;

                if (_format == WAVE_FORMAT_PCM && _bitwidth == 24 && _blockalign == 4 * _channels)
                {
                    /* Invalid file generated by Syntrillium's Cooledit!
                     * Treating as WAVE_FORMAT_IEEE_FLOAT 32 bit floating point file.
                     * Bit Width     : 24 (should be 32)
                     */
                    _format = WAVE_FORMAT_IEEE_FLOAT;
                    return false;
                }

                _blockwidth = _bytewidth * _channels;

                break;

            case data_MARKER:
                //qDebug() << "data_MARKER";

                if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
                    return false;

                parsestage |= HAVE_data;

                if (MusicFile::_readData(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = FromEndian(_endian, chunk_size);

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
                //qDebug() << "_dataBegin" << _dataBegin << "_dataSize" << _dataSize;
                MusicFile::seek(_dataEnd);

                break;

            case LIST_MARKER:
            case cue_MARKER:
            case iwep_MARKER:
            case iwem_MARKER:
            case acid_MARKER:
            case inst_MARKER:
            case Appl_MARKER:
                //qDebug() << "LIST_MARKER | cue_MARKER | iwep_MARKER | iwem_MARKER | acid_MARKER | Appl_MARKER";
                parsestage |= HAVE_other;
                if (MusicFile::_readData(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = FromEndian(_endian, chunk_size);
                //qDebug() << "chunk_size" << chunk_size;

                int32 = MusicFile::_pos() + chunk_size;
                if (int32 > MusicFile::size())
                    return false;
                MusicFile::seek(int32);

                break;

            default:
                //qDebug() << "other_MARKER" << QString::number(marker, 16);
                return false;
                parsestage |= HAVE_other;
                if (MusicFile::_readData(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)) != sizeof(chunk_size))
                    return false;
                chunk_size = FromEndian(_endian, chunk_size);

                int32 = MusicFile::_pos() + chunk_size;
                //qDebug() << MusicFile::_pos() << chunk_size << int32 << MusicFile::size();
                if (int32 > MusicFile::size())
                    return false;
                //return false;
                MusicFile::seek(int32);

                break;
        }
        if (MusicFile::_pos() >= MusicFile::size() - 4)
            break;
    }
    //qDebug() << _dataBegin;
    if (!_dataBegin)
        return false;
    return true;
}

MusicFile_Wav::MusicFile_Wav(const MusicData& fileDescription) :
    MusicFile(fileDescription),
    _endian(_LittleEndian),
    _dataBegin(0),
    _dataSize(0),
    _dataEnd(0)
{
    //qDebug() << Q_FUNC_INFO << MusicFile::fileName();
}

bool MusicFile_Wav::open(OpenMode mode)
{
    //qDebug() << Q_FUNC_INFO << mode;
    if (!MusicFile::open(mode))
        return false;
    if ((mode & ReadOnly) == ReadOnly)
    {
        if (!_parseHeader())
        {
            //Assuming as raw data
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
        seek(0);
        //remove leading zeros
        qint16* samples = new qint16[_channels];
        bool zero = true;
        while (zero && MusicFile::_readData(reinterpret_cast<char*>(samples), _blockwidth) == _blockwidth)
        {
            for (uint i = 0; i < _channels; ++i)
            {
                if (samples[i] != 0)
                {
                    zero = false;
                    break;
                }
            }
            if (zero)
            {
                _dataBegin += _blockwidth;
                _dataSize -= _blockwidth;
                --_loopBegin;
                --_loopEnd;
            }
        }
        seek(0);
    }
    return true;
}

qint64 MusicFile_Wav::size() const
{
    //qDebug() << Q_FUNC_INFO;
    return _dataSize;
}

bool MusicFile_Wav::seek(qint64 pos)
{
    //qDebug() << Q_FUNC_INFO << pos;
    return QIODevice::seek(pos) && MusicFile::_seek(pos + _dataBegin);
}

bool MusicFile_Wav::reset()
{
    //qDebug() << Q_FUNC_INFO;
    return MusicFile::reset() && seek(0);
}

qint64 MusicFile_Wav::readData(char* data, qint64 maxSize)
{
    //qDebug() << Q_FUNC_INFO << maxSize;
    if (MusicFile::_pos() < _dataBegin)
    {
        //qDebug() << Q_FUNC_INFO << "MusicFile::_pos() < _dataBegin" << _dataBegin;
        MusicFile::seek(_dataBegin);
    }
    if (MusicFile::_pos() > _dataEnd)
    {
        //qDebug() << Q_FUNC_INFO << "MusicFile::_pos() > _dataEnd" << _dataEnd;
        MusicFile::seek(_dataEnd);
    }
    //qDebug() << Q_FUNC_INFO << MusicFile::_pos();
    if (maxSize > _dataEnd - MusicFile::_pos())
        maxSize = _dataEnd - MusicFile::_pos();
    //qDebug() << Q_FUNC_INFO << maxSize;
    return MusicFile::readData(data, maxSize);
}
