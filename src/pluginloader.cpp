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
#include <QApplication>
#include <QDir>
#include <QByteArray>
#include <QPluginLoader>

#include "pluginloader.h"

PluginLoader::PluginLoader()
{
    QDir pluginsDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS")
    {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("plugins");

    foreach (QString fileName, pluginsDir.entryList(QDir::Files))
    {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (plugin)
        {
            LoaderInterface* loaderinterface = qobject_cast<LoaderInterface *>(plugin);
            if (loaderinterface)
            {
                loader_list_map.insert(loaderinterface->title(), loader_list.size());
                loader_list << loaderinterface;
            }
        }
    }
}

void PluginLoader::clear()
{
    data.clear();
}

bool PluginLoader::load(QString title, QString path)
{
    if (!loader_list_map.contains(title))
        return false;

    LoaderInterface* dataLoader = loader_list.at(loader_list_map.value(title));
    {
        /*
        QTime time;
        time.start();
        for (int i = 0; i < 1000; ++i)
        {
            dataLoader->open(path);
            dataLoader->close();
        }
        int t = time.elapsed();
        qDebug() << dataLoader->title() << " used " << t * 0.001 << " ms";
        */
        if (!dataLoader->open(path))
            return false;
    }
    for (uint i = 0; i < dataLoader->size(); ++i)
    {
        MusicData musicData(dataLoader->at(i));
        data << musicData;
    }
    dataLoader->close();
    return true;
}

