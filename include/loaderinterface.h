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
#ifndef LOADERINTERFACE_H
#define LOADERINTERFACE_H
#include <QtPlugin>
#include "musicdata.h"

class LoaderInterface
{
    public:
        virtual ~LoaderInterface() {}
        virtual const QString& title() const = 0;
        virtual bool open(const QString &) = 0;
        virtual void close() = 0;
        virtual MusicData at(uint index) = 0;
        virtual uint size() const = 0;
};
Q_DECLARE_INTERFACE(LoaderInterface, "org.BestSteve.touhoumusicplayer.LoaderInterface/1.0")

#endif // LOADERINTERFACE_H
