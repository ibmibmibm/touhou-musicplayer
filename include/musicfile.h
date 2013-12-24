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
#ifndef MUSICFILE_H
#define MUSICFILE_H
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QString>
#include <QByteArray>
#include <QExplicitlySharedDataPointer>
#include <phonon/mediaobject.h>
#include "musicdata.h"

class MusicFile
{
    private:
        class MusicFileData : public QSharedData
        {
            private:
                MusicFileData(const MusicFileData &other);
                MusicFileData&operator=(const MusicFileData &other);
            public:
                MusicFileData(const MusicData& data, const QString path):
                    title(data.title),
                    album(data.album),
                    loop(data.loop),
                    loopStart(data.loopStart),
                    loopEnd(data.loopEnd)
                {
                    QDir dir(path);
                    dir.mkpath(album);
                    QDir adir(dir.filePath(album));
                    fileName = adir.filePath(data.title + data.suffix);
                    QFile file(fileName);
                    if (file.size() != data.content.size())
                    {
                        if (!file.open(QIODevice::WriteOnly))
                            return;
                        file.write(data.content);
                        file.close();
                    }
                    qDebug() << fileName;
                }
                QString title;
                QString album;
                QString fileName;
                bool loop;
                uint loopStart;
                uint loopEnd;
        };
    public:
        MusicFile(const MusicData& data, const QString& path) :
            data(new MusicFileData(data, path))
            {}
        bool isLoop() const { return this->data->loop; }
        uint loopStart() const { return this->data->loopStart; }
        uint loopEnd() const { return this->data->loopEnd; }
        Phonon::MediaSource source() const { return Phonon::MediaSource(this->data->fileName); }
        const QString& title() const { return this->data->title; }
        void setTitle(const QString& title) { this->data->title = title; }
        const QString& album() const { return this->data->album; }
        void setAlbum(const QString& album) { this->data->album = album; }
    private:
        MusicFile();
        QExplicitlySharedDataPointer<MusicFileData> data;
};

#endif //MUSICFILE_H
