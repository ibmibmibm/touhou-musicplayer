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
#include <QLocale>

#include "mainwindow.h"

int main(int argv, char **args)
{
    QApplication app(argv, args);
    app.setOrganizationName("Touhou Music Player");
    app.setApplicationName("Touhou Music Player");
    app.setQuitOnLastWindowClosed(true);

    QString locale = QLocale::system().name();

    QTranslator translator;
    translator.load(QString("touhou_musicplayer_") + locale);
    app.installTranslator(&translator);

    MainWindow window;
    window.show();

    return app.exec();
}
