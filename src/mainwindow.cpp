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
#include <QPluginLoader>
#include <QSettings>
#include <QTime>
#include <QMessageBox>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>

#include "mainwindow.h"

namespace {
    QString RepeatString(int life)
    {
        if (life > 10)
            return QString::fromWCharArray(L"\u2605 x %1").arg(life);
        return QString(life, L'\u2605');
    }
}

MainWindow::MainWindow()
{
    musicPlayer = new MusicPlayer();
    pluginLoader = new PluginLoader();
    connect(musicPlayer, SIGNAL(stateChanged(MusicPlayerState, MusicPlayerState)),
            this, SLOT(stateChanged(MusicPlayerState, MusicPlayerState)));
    connect(musicPlayer, SIGNAL(tick(int)), this, SLOT(tick(int)));
    connect(musicPlayer, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
    connect(musicPlayer, SIGNAL(currentMusicChanged(const MusicFile&)), this, SLOT(currentMusicChanged(const MusicFile&)));
    connect(musicPlayer, SIGNAL(remainLoopChanged(int)), this, SLOT(remainLoopChanged(int)));

    connect(pluginLoader, SIGNAL(loadProgress(int)), this, SLOT(loadProgress(int)));

    setupActions();
    setupMenus();
    setupUi();
    timeLcd->display("00:00.000");
}

MainWindow::~MainWindow()
{
    delete pluginLoader;
    pluginLoader = NULL;
    delete musicPlayer;
    musicPlayer = NULL;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    musicPlayer->stop();
    musicPlayer->clearQueue();
    event->accept();
}

void MainWindow::config()
{
    QSettings settings;
    int size = settings.beginReadArray("appDir");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        QString title = settings.value("title").toString();
        QString path = settings.value("path").toString();
        if (pluginLoader->contains(title))
        {
            int id = pluginLoader->id(title);
            configDialog->setDir(id, path);
        }
    }
    settings.endArray();
    if (configDialog->exec() == QDialog::Accepted)
    {
        settings.beginWriteArray("appDir");
        for (int i = 0; i < pluginLoader->size(); ++i)
        {
            settings.setArrayIndex(i);
            settings.setValue("title", pluginLoader->title(i));
            settings.setValue("path", configDialog->dir(i));
        }
        settings.endArray();
    }
}

void MainWindow::insertMusic(const MusicFile& music)
{
    musics << music;
    int r = musicTable->rowCount();
    musicTable->insertRow(r);

    QSpinBox *spin = new QSpinBox(this);
    musicTable->setCellWidget(r, 0, spin);
    spin->setValue(2);

    QTableWidgetItem *titleItem = new QTableWidgetItem(music.title());
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
    musicTable->setItem(r, 1, titleItem);

    QTableWidgetItem *albumItem = new QTableWidgetItem(music.album());
    albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);
    musicTable->setItem(r, 2, albumItem);

    musicTable->selectRow(r);
    musicTable->resizeRowsToContents();
}

void MainWindow::loadFile()
{
    pluginLoader->clear();
    QSettings settings;

    int size = settings.beginReadArray("appDir");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        QString title = settings.value("title").toString();
        QString path = settings.value("path").toString();
        loadingTitle = title;

        if (!pluginLoader->load(title, path, "music"))
            QMessageBox::warning(this, tr("Fatal Error"), tr("%1 is not the installation path of %2.").arg(path).arg(title));
    }
    settings.endArray();

    while (musicTable->rowCount() > 0)
        musicTable->removeRow(0);
    musics.clear();

    for (int i = 0; i < pluginLoader->musicSize(); ++i)
        insertMusic(MusicFile(pluginLoader->musicData(i)));

    if (musics.size())
    {
        musicTable->setEnabled(true);

        currentIndex = 0;
        musicPlayer->setCurrentMusic(musics.at(0), qobject_cast<QSpinBox*>(musicTable->cellWidget(0, 0))->value());

        nextAction->setEnabled(true);
        previousAction->setEnabled(true);
    }
}

void MainWindow::about()
{
    musicTable->resizeRowsToContents();
    qDebug() << musicTable->columnWidth(0) << musicTable->columnWidth(1) << musicTable->columnWidth(2);
    QMessageBox::about(this, tr("About Touhou Music Player"), tr(
        "Touhou Music Player %1 by BestSteve (ibmibmibm - ptt.cc)\n\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 3 of the License, or\n"
        "(at your option) any later version.\n\n"

        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n\n"

        "You should have received a copy of the GNU General Public License\n"
        "along with this program.  If not, see <http://www.gnu.org/licenses/>."
    ).arg(qApp->applicationVersion()));
}

void MainWindow::loadProgress(int progress)
{
    titleLabel->setText(QString("%1 %2%").arg(loadingTitle).arg(progress));
    qApp->processEvents();
}

void MainWindow::stateChanged(MusicPlayerState newState, MusicPlayerState /* oldState */)
{
    switch (newState) {
        case ErrorState:
            if (musicPlayer->errorType() == FatalError)
                QMessageBox::warning(this, tr("Fatal Error"), musicPlayer->errorString());
            else
                QMessageBox::warning(this, tr("Error"), musicPlayer->errorString());
            break;
        case PlayingState:
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            break;
        case StoppedState:
            stopAction->setEnabled(false);
            playAction->setEnabled(true);
            pauseAction->setEnabled(false);
            timeLcd->display("00:00.000");
            break;
        case PausedState:
            pauseAction->setEnabled(false);
            stopAction->setEnabled(true);
            playAction->setEnabled(true);
            break;
        default:
            ;
    }
}

void MainWindow::totalSamplesChanged(int newTotalSamples)
{
    seekSlider->setMaximum(newTotalSamples);
//    qDebug() << newTotalSamples;
}

void MainWindow::remainLoopChanged(int newRemainLoop)
{
    titleLabel->setText(QString("%1 %2").arg(musics.at(currentIndex).title()).arg(RepeatString(newRemainLoop)));
}

void MainWindow::tick(int samples)
{
    int time = samples * 0.02267573696145124716553287982 + 0.5;
    int msec = time % 1000;
    time /= 1000;
    int sec = time % 60;
    time /= 60;
    int min = time % 60;
    QTime displayTime(0, min, sec, msec);
    timeLcd->display(displayTime.toString("mm:ss.zzz"));
    seekSlider->setValue(samples);
}

void MainWindow::currentMusicChanged(const MusicFile& music)
{
    currentIndex = musics.indexOf(music);
    musicTable->selectRow(currentIndex);
    titleLabel->setText(QString("%1 %2").arg(musics.at(currentIndex).title()).arg(RepeatString(musicPlayer->remainLoop())));
    timeLcd->display("00:00.000");
}

int MainWindow::getNewId(int offset)
{
    offset %= musics.size();
    if (offset < 0)
        offset += musics.size();
    int ret = currentIndex;
    do
    {
        ret = (ret+offset) % musics.size();
        if (ret == currentIndex)
            break;
    }
    while (qobject_cast<QSpinBox*>(musicTable->cellWidget(ret, 0))->value() == 0);
    return ret;
}

void MainWindow::aboutToFinish()
{
    Q_ASSERT(musics.size() > currentIndex);
    int next = getNewId(1);
    musicPlayer->enqueue(musics.at(next), qobject_cast<QSpinBox*>(musicTable->cellWidget(next, 0))->value());
}

void MainWindow::next()
{
    musicChanged(getNewId(1));
}

void MainWindow::previous()
{
    musicChanged(getNewId(-1));
}

void MainWindow::musicChanged(int row, int /*column*/)
{
    bool wasPlaying = musicPlayer->state() == PlayingState;

    musicPlayer->stop();
    musicPlayer->clearQueue();

    musicPlayer->setCurrentMusic(musics.at(row), qobject_cast<QSpinBox*>(musicTable->cellWidget(row, 0))->value());
    titleLabel->setText(QString("%1 %2").arg(musics.at(currentIndex).title()).arg(RepeatString(musicPlayer->remainLoop())));

    if (wasPlaying)
        musicPlayer->play();
    else
        musicPlayer->stop();
}

void MainWindow::setupActions()
{
    playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("&Play"), this);
    playAction->setShortcut(tr("Ctrl+P"));
    playAction->setDisabled(true);
    pauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPause), tr("P&ause"), this);
    pauseAction->setShortcut(tr("Ctrl+A"));
    pauseAction->setDisabled(true);
    stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("&Stop"), this);
    stopAction->setShortcut(tr("Ctrl+S"));
    stopAction->setDisabled(true);
    nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("&Next"), this);
    nextAction->setShortcut(tr("Ctrl+N"));
    nextAction->setDisabled(true);
    previousAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("P&revious"), this);
    previousAction->setShortcut(tr("Ctrl+R"));
    previousAction->setDisabled(true);
    loadFileAction = new QAction(tr("&Load data file"), this);
    loadFileAction->setShortcut(tr("Ctrl+L"));
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+X"));
    configAction = new QAction(tr("Con&fig"), this);
    configAction->setShortcut(tr("Ctrl+F"));
    aboutAction = new QAction(tr("A&bout"), this);
    aboutAction->setShortcut(tr("Ctrl+B"));
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setShortcut(tr("Ctrl+Q"));

    connect(playAction, SIGNAL(triggered()), musicPlayer, SLOT(play()));
    connect(pauseAction, SIGNAL(triggered()), musicPlayer, SLOT(pause()) );
    connect(stopAction, SIGNAL(triggered()), musicPlayer, SLOT(stop()));
    connect(nextAction, SIGNAL(triggered()), this, SLOT(next()));
    connect(previousAction, SIGNAL(triggered()), this, SLOT(previous()));
    connect(loadFileAction, SIGNAL(triggered()), this, SLOT(loadFile()));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(configAction, SIGNAL(triggered()), this, SLOT(config()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(loadFileAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(configAction);

    QMenu *playbackMenu = menuBar()->addMenu(tr("&Playback"));
    playbackMenu->addAction(playAction);
    playbackMenu->addAction(pauseAction);
    playbackMenu->addAction(stopAction);
    playbackMenu->addSeparator();
    playbackMenu->addAction(nextAction);
    playbackMenu->addAction(previousAction);

    QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutAction);
    aboutMenu->addAction(aboutQtAction);
}

void MainWindow::setupUi()
{
    configDialog = new ConfigDialog(pluginLoader->title(), this);

    QToolBar *bar = new QToolBar(this);

    bar->addAction(playAction);
    bar->addAction(pauseAction);
    bar->addAction(stopAction);
    bar->addAction(previousAction);
    bar->addAction(nextAction);

    seekSlider = new QSlider(Qt::Horizontal, this);
    connect(musicPlayer, SIGNAL(totalSamplesChanged(int)), this, SLOT(totalSamplesChanged(int)));
    connect(seekSlider, SIGNAL(sliderMoved(int)), musicPlayer, SLOT(seek(int)));

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setMaximum(128);
    volumeSlider->setValue(128);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(volumeSlider, SIGNAL(valueChanged(int)), musicPlayer, SLOT(setVolume(int)));
    connect(musicPlayer, SIGNAL(volumeChanged(int)), volumeSlider, SLOT(setValue(int)));

    titleLabel = new QLabel(this);

    QPalette palette;
    palette.setBrush(QPalette::Light, Qt::darkGray);

    timeLcd = new QLCDNumber(this);
    timeLcd->setPalette(palette);
    timeLcd->setNumDigits(9);

    QStringList title;
    title << tr("Repeat") << tr("Title") << tr("Album");

    musicTable = new QTableWidget(0, title.size(), this);
    musicTable->setHorizontalHeaderLabels(title);
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(musicTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(musicChanged(int, int)));

    musicTable->setColumnWidth(0, 56);
    musicTable->setColumnWidth(1, 251);
    musicTable->setColumnWidth(2, 295);
    musicTable->setEnabled(false);

    QHBoxLayout *seekerLayout = new QHBoxLayout();
    seekerLayout->addWidget(seekSlider);
    seekerLayout->addWidget(timeLcd);

    QHBoxLayout *playbackLayout = new QHBoxLayout();
    playbackLayout->addWidget(bar);
    playbackLayout->addWidget(titleLabel, 1);
    playbackLayout->addWidget(volumeSlider);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(seekerLayout);
    mainLayout->addLayout(playbackLayout);
    mainLayout->addWidget(musicTable);

    QWidget *widget = new QWidget(this);
    widget->setLayout(mainLayout);

    setCentralWidget(widget);
    setWindowTitle(tr("Touhou Music Player"));
}
