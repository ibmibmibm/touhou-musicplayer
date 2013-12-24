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

inline quint8 getUInt8(const char *cursor)
{
    return *cursor;
}

inline quint16 getUInt16(const char *cursor)
{
    return qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(cursor));
}

inline quint32 getUInt32(const char *cursor)
{
    return qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(cursor));
}

inline quint64 getUInt64(const char *cursor)
{
    return qFromLittleEndian<qint64>(reinterpret_cast<const uchar*>(cursor));
}

inline void setUInt8(quint8 value, char *cursor)
{
    *cursor = value;
}

inline void setUInt16(quint16 value, char *cursor)
{
    qToLittleEndian<qint16>(value, reinterpret_cast<uchar*>(cursor));
}

inline void setUInt32(quint32 value, char *cursor)
{
    qToLittleEndian<qint32>(value, reinterpret_cast<uchar*>(cursor));
}

inline void setUInt64(quint64 value, char *cursor)
{
    qToLittleEndian<qint64>(value, reinterpret_cast<uchar*>(cursor));
}

#endif // HELPERFUNCS_H
