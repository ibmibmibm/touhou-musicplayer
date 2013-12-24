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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QList>

#include "musicplayer.h"
#include "configdialog.h"
#include "musicdata.h"

class QAction;
class QTableView;
class QTableWidget;
class QLCDNumber;
class QLabel;

class PluginLoader;
class PlaylistModel;
class SpinBoxDelegate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow();
        ~MainWindow();

        QSize sizeHint() const { return QSize(800, 600); }

    protected:
        void closeEvent(QCloseEvent *event);

    private slots:
        void loadFile();
        void saveFile();
        void config();
        void about();
        void next();
        void previous();
        void stateChanged(MusicPlayerState newState, MusicPlayerState oldState);
        void totalSamplesChanged(qint64 newTotalSamples);
        void loopChanged(uint newLoop);
        void tick(qint64 sample);
        void currentMusicChanged(const MusicData& musicData);
        void aboutToFinish();
        void musicChanged(int row, int column = 0);
        void playlistDoubleClicked(const QModelIndex& index);
        void seek(int newValue) { musicPlayer->seek(newValue); }
        void setVolume(int newVolume) { musicPlayer->setVolume(newVolume * 0.0078125); }
        void setVolume(qreal newVolume) { volumeSlider->setValue(newVolume * 128.0); }

    private:
        int getNewId(int offset);
        void setupActions();
        void setupMenus();
        void setupUi();
        void _loadSettingLoadOnStartup();
        void _loadSettingPluginLoaderPath();
        void _loadSettingPlaylist();

        QSlider *seekSlider;
        QSlider *volumeSlider;
        QLabel *titleLabel;
        QTableView *playlistTableView;
        int currentIndex;

        PluginLoader *pluginLoader;
        MusicPlayer *musicPlayer;
        PlaylistModel *playlistModel;
        SpinBoxDelegate *spinBoxDelegate;
        QString loadingTitle;

        QAction *playAction;
        QAction *pauseAction;
        QAction *stopAction;
        QAction *nextAction;
        QAction *previousAction;
        QAction *loadFileAction;
        QAction *saveFileAction;
        QAction *exitAction;
        QAction *configAction;
        QAction *aboutAction;
        QAction *aboutQtAction;
        QLCDNumber *timeLcd;
};

#endif //MAINWINDOW_H
