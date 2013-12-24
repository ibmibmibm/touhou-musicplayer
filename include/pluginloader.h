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
#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H
#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include "musicdata.h"
#include "loaderinterface.h"

class PluginLoader : public QObject
{
    Q_OBJECT

    public:
        PluginLoader();
        void clear();
        bool load(QString title, QString path);
        bool contains(QString title) const { return loader_list_map.contains(title); }
        int id(QString title) const { return loader_list_map.value(title); }
        QString title(int id) const { return loader_list.at(id)->title(); }
        int size() const { return loader_list.size(); }
        int musicSize() const { return data.size(); }
        const MusicData& musicData(int idx) const { return data.at(idx); }
    private:
        QList<LoaderInterface*> loader_list;
        QHash<QString, int> loader_list_map;
        QList<MusicData> data;
};

#endif // PLUGINLOADER_H
