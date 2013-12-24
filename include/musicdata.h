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
#ifndef MUSICDATA_H
#define MUSICDATA_H
#include <QString>
#include <QByteArray>

struct MusicData
{
    MusicData(QByteArray _content, QString _suffix, QString _title, QString _album, bool _loop, uint _loopStart, uint _loopEnd) :
        content(_content),
        suffix(_suffix),
        title(_title),
        album(_album),
        loop(_loop),
        loopStart(_loopStart),
        loopEnd(_loopEnd)
        {}
    QByteArray content;
    QString suffix;
    QString title;
    QString album;
    bool loop;
    uint loopStart;
    uint loopEnd;
};

#endif // MUSICDATA_H
