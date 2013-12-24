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
#include "musicsaver.h"

QHash<QString, MusicSaverFactory::CreateFunction> MusicSaverFactory::functionHash;

int MusicSaverFactory::registerMusicSaver(const QString& filterString, CreateFunction createFunction)
{
    functionHash.insert(filterString, createFunction);
    return functionHash.size();
}

MusicSaver* MusicSaverFactory::createMusicSaver(const QString& filterString)
{
    Q_ASSERT(functionHash.contains(filterString));
    return functionHash.value(filterString)();
}

QStringList MusicSaverFactory::filterStringList()
{
    QStringList filterList(functionHash.keys());
    return filterList;
}
