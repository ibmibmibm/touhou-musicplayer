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
#ifndef TH06LOADER_H
#define TH06LOADER_H
#include <QObject>
#include <QString>
#include <QList>
#include <QDir>
#include "loaderinterface.h"

struct MusicInfo
{
    quint32 loopBegin;
    quint32 loopEnd;
};

class Th06Loader : public QObject, public LoaderInterface
{
    Q_OBJECT
    Q_INTERFACES(LoaderInterface)

    public:
        Th06Loader();
        const QString& title() const;
        bool open(const QString &);
        void close();
        MusicData at(uint index);
        uint size() const;
    private:
        QList<MusicInfo> musicInfoList;
        QDir programDirectory;
};

#endif //TH06LOADER_H
