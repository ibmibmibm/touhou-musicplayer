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

#include <Q_INT8>
#include <Q_INT16>
#include <Q_INT32>
#include <Q_INT64>

inline quint8 getUInt8(const char *cursor)
{
    quint8 u = *reinterpret_cast<const quint8*>(cursor);
    return u;
}

inline quint16 getUInt16(const char *cursor)
{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    const unsigned char * c = reinterpret_cast<const unsigned char*>(cursor);
    quint16 u = c[1];
    u <<= 8;
    u += c[0];
    return u;
#else
    quint16 u = *reinterpret_cast<const quint16*>(cursor);
    return u;
#endif
}

inline quint32 getUInt32(const char *cursor)
{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    const unsigned char * c = reinterpret_cast<const unsigned char*>(cursor);
    quint32 u = c[3];
    u <<= 8;
    u += c[2];
    u <<= 8;
    u += c[1];
    u <<= 8;
    u += c[0];
    return u;
#else
    quint32 u = *reinterpret_cast<const quint32*>(cursor);
    return u;
#endif
}

inline quint64 getUInt64(const char *cursor)
{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    const unsigned char * c = reinterpret_cast<const unsigned char*>(cursor);
    quint64 u = c[7];
    u <<= 8;
    u += c[6];
    u <<= 8;
    u += c[5];
    u <<= 8;
    u += c[4];
    u <<= 8;
    u += c[3];
    u <<= 8;
    u += c[2];
    u <<= 8;
    u += c[1];
    u <<= 8;
    u += c[0];
    return u;
#else
    quint64 u = *reinterpret_cast<const quint64*>(cursor);
    return u;
#endif
}

#endif // HELPERFUNCS_H
