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
#ifndef MUSICSAVER_H
#define MUSICSAVER_H
#include <QObject>
#include "musicdata.h"

class MusicSaver
{
    public:
        virtual bool save(const QString& filename, MusicData musicData, uint loop, uint fadeoutTime) = 0;
        virtual ~MusicSaver() {}
        QString errorString() const { return _errorString; }
    protected:
        qreal fadeoutVolume(quint64 fadeoutSample, quint64 sample);
        void setErrorString(QString newErrorString) { _errorString = newErrorString; }
    private:
        QString _errorString;
};

#endif // MUSICSAVER_H
