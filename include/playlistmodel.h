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
#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include "musicdata.h"

struct PlaylistItem
{
    uint totalLoopCount;
    MusicData musicData;
};

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        PlaylistModel();
        virtual ~PlaylistModel();
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        virtual bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
        virtual bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

        MusicData musicData(int index) const;
        void setMusicData(int index, const MusicData &musicData);
        uint loop(int index) const;
        void setLoop(int index, const uint &loop);
        int indexOf(const MusicData &musicData) const;
    private:
        QList<PlaylistItem> musicDataList;
};

#endif //PLAYLISTMODEL_H
