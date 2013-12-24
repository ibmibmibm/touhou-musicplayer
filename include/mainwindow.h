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
#include <phonon/phononnamespace.h>
#include <phonon/audiooutput.h>
#include <phonon/seekslider.h>
#include <phonon/mediaobject.h>
#include <phonon/volumeslider.h>
#include <phonon/backendcapabilities.h>
#include <QHash>
#include <QList>
#include <QAction>
#include <QTableWidget>
#include <QLCDNumber>
#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>

#include "musicfile.h"
#include "loaderinterface.h"
#include "configdialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    QSize sizeHint() const {
        return QSize(800, 600);
    }

protected:
    void closeEvent(QCloseEvent *event);
	
private slots:
    void loadFile();
    void config();
    void about();
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void tick(qint64 time);
    void sourceChanged(const Phonon::MediaSource &source);
    void aboutToFinish();
    void musicChanged(int row, int = 0);
    void next();
    void previous();
    void insertMusic(const MusicFile& music);

private:
    int getNewId(int offset);
    void setupActions();
    void setupMenus();
    void setupUi();
    void setupPlugins();

    Phonon::SeekSlider *seekSlider;
    Phonon::MediaObject *mediaObject;
    Phonon::AudioOutput *audioOutput;
    Phonon::VolumeSlider *volumeSlider;
    QLabel *titleLabel;
    QTableWidget *musicTable;
    QList<LoaderInterface*> loaders;
    QHash<QString, int> loaders_map;
    QList<MusicFile> musics;
    int currentIndex;
    int nextIndex;
    int currentLoop;

    QAction *playAction;
    QAction *pauseAction;
    QAction *stopAction;
    QAction *nextAction;
    QAction *previousAction;
    QAction *loadFileAction;
    QAction *exitAction;
    QAction *configAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
    QLCDNumber *timeLcd;
    ConfigDialog *configDialog;
};

#endif //MAINWINDOW_H
