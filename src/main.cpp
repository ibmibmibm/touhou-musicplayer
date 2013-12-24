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
#include <QString>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QLibraryInfo>

#include "mainwindow.h"
#include "musicfile_ogg.h"
#include "musicfile_wav.h"
#include "musicsaver_wav.h"
#include "musicsaver_flac.h"

const uint VERSION = 0x00070000;

void updateSettings()
{
    QSettings settings;
    uint prevVersion = settings.value(QLatin1String("Version")).toUInt();
    if (prevVersion == VERSION)
        return;
    switch (prevVersion)
    {
        case 0:
            {
                QList< QPair<QString, QString> > appDir;
                int size = settings.beginReadArray("appDir");
                for (int i = 0; i < size; ++i)
                {
                    settings.setArrayIndex(i);
                    QString title = settings.value("title").toString();
                    QString path = settings.value("path").toString();
                    appDir << qMakePair(title, path);
                }
                settings.endArray();

                settings.remove("appDir");

                settings.beginGroup("General");
                settings.beginWriteArray("Applications Directory");
                for (int i = 0; i < size; ++i)
                {
                    settings.setArrayIndex(i);
                    settings.setValue("Title", appDir.at(i).first);
                    settings.setValue("Path", appDir.at(i).second);
                }
                settings.endArray();
                settings.endGroup();
                prevVersion = 0x00030001;
            }
        case 0x00030001:
            prevVersion = 0x00040000;
        case 0x00040000:
            prevVersion = 0x00050000;
        case 0x00050000:
            prevVersion = 0x00060000;
        case 0x00060000:
            prevVersion = 0x00070000;
            break;
        default:
            qWarning("Unknow version.");
            settings.clear();
    }
    settings.setValue(QLatin1String("Version"), VERSION);
}

void initialFactories()
{
    MusicFileFactory::registerMusicFile(".ogg", MusicFile_Ogg::createFunction);
    MusicFileFactory::registerMusicFile(".wav", MusicFile_Wav::createFunction);
    MusicSaverFactory::registerMusicSaver(MusicSaver_Wav::filterString(), MusicSaver_Wav::createFunction);
    MusicSaverFactory::registerMusicSaver(MusicSaver_Flac::filterString(), MusicSaver_Flac::createFunction);
}

int main(int argv, char **args)
{
    QApplication app(argv, args);
    app.setOrganizationName("Touhou Music Player");
    app.setApplicationName("Touhou Music Player");
    app.setApplicationVersion(QString("%1.%2.%3")
        .arg((VERSION >> 24) & 0xff)
        .arg((VERSION >> 16) & 0xff)
        .arg(VERSION & 0xffff));
    app.setQuitOnLastWindowClosed(true);

    QTranslator translator;
    QTranslator qtTranslator;
    {
        QString resourceDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        //qDebug() << Q_FUNC_INFO << "resourceDir" << resourceDir;

        QString locale = QLocale::system().name();
        translator.load("touhou_musicplayer_" + locale, resourceDir);
        qtTranslator.load("qt_" + locale, resourceDir);
    }
    app.installTranslator(&translator);
    app.installTranslator(&qtTranslator);

    updateSettings();
    initialFactories();

    MainWindow window;
    window.show();

    return app.exec();
}
