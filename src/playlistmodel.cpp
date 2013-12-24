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
#include "playlistmodel.h"

PlaylistModel::PlaylistModel()
{
}

PlaylistModel::~PlaylistModel()
{
}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const
{
    //qDebug() << Q_FUNC_INFO << musicDataList.count();
    return musicDataList.count();
}

int PlaylistModel::columnCount(const QModelIndex& /*parent*/) const
{
    //qDebug() << Q_FUNC_INFO;
    return 3;
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    if (index.column() == 0)
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    return QAbstractItemModel::flags(index);
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row() >= musicDataList.size())
        return QVariant();
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();
    //qDebug() << Q_FUNC_INFO << index.row() << ',' << index.column();
    switch (index.column())
    {
        case 0:
            return QVariant(musicDataList.at(index.row()).totalLoopCount);
        case 1:
            return QVariant(musicDataList.at(index.row()).musicData.title());
        case 2:
            return QVariant(musicDataList.at(index.row()).musicData.album());
        default:
            return QVariant();
    }
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        switch (index.column())
        {
            case 0:
                //qDebug() << Q_FUNC_INFO << index.row() << ',' << index.column() << ',' << value.toUInt();
                musicDataList[index.row()].totalLoopCount = value.toUInt();
                emit dataChanged(index, index);
                return true;
            default:
                return false;
        }
    }
    return false;
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //qDebug() << Q_FUNC_INFO << section << ',' << orientation;
    if (role != Qt::DisplayRole)
        return QVariant();
    switch (orientation)
    {
        case Qt::Vertical:
            return QString("%1").arg(section);
        case Qt::Horizontal:
            switch (section)
            {
                case 0:
                    return tr("Repeat");
                case 1:
                    return tr("Title");
                case 2:
                    return tr("Album");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
}

bool PlaylistModel::insertRows(int position, int rows, const QModelIndex &/*index*/)
{
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row)
        musicDataList.insert(position, PlaylistItem());

    endInsertRows();
    return true;
}

bool PlaylistModel::removeRows(int position, int rows, const QModelIndex &/*index*/)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row)
        musicDataList.removeAt(position);

    endRemoveRows();
    return true;
}

MusicData PlaylistModel::musicData(int index) const
{
    return musicDataList.at(index).musicData;
}

void PlaylistModel::setMusicData(int index, const MusicData &musicData)
{
    musicDataList[index].musicData = musicData;
}

uint PlaylistModel::loop(int index) const
{
    return musicDataList.at(index).totalLoopCount;
}

void PlaylistModel::setLoop(int index, const uint &loop)
{
    musicDataList[index].totalLoopCount = loop;
}

int PlaylistModel::indexOf(const MusicData &musicData) const
{
    for (int i = 0; i < musicDataList.size(); ++i)
    {
        if (musicDataList.at(i).musicData == musicData)
            return i;
    }
    return -1;
}
