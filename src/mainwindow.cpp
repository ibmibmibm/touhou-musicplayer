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
#include <QSettings>
#include <QTime>
#include <QAction>
#include <QTableView>
#include <QHeaderView>
#include <QLCDNumber>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QFileDialog>

#include "pluginloader.h"
#include "musicsaver.h"
#include "playlistmodel.h"
#include "spinboxdelegate.h"
#include "mainwindow.h"

namespace {
    inline QString RepeatString(int life)
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
    connect(musicPlayer, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(musicPlayer, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
    connect(musicPlayer, SIGNAL(currentMusicChanged(const MusicData&)), this, SLOT(currentMusicChanged(const MusicData&)));
    connect(musicPlayer, SIGNAL(loopChanged(uint)), this, SLOT(loopChanged(uint)));
    playlistModel = new PlaylistModel();
    spinBoxDelegate = new SpinBoxDelegate(this);

    setupActions();
    setupMenus();
    setupUi();

    _loadSettingLoadOnStartup();
}

MainWindow::~MainWindow()
{
    delete pluginLoader;
    pluginLoader = NULL;
    delete musicPlayer;
    musicPlayer = NULL;
    delete spinBoxDelegate;
    spinBoxDelegate = NULL;
    delete playlistModel;
    playlistModel = NULL;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    musicPlayer->stop();
    musicPlayer->clearQueue();
    {
        QSettings settings;
        settings.beginGroup("Playlist");
        settings.beginWriteArray("Items");
        for (int i = 0; i < playlistModel->rowCount(); ++i)
        {
            settings.setArrayIndex(i);
            settings.setValue("Visual Index", playlistTableView->verticalHeader()->visualIndex(i));
            settings.setValue("Repeat", playlistModel->loop(i));
        }
        settings.endArray();
        settings.endGroup();
    }
    event->accept();
}

void MainWindow::config()
{
    ConfigDialog(pluginLoader, musicPlayer, this).exec();
}

void MainWindow::loadFile()
{
    pluginLoader->clear();

    //QTime time;
    //time.restart();

    _loadSettingPluginLoaderPath();

    //qDebug() << Q_FUNC_INFO << "Loading used " << time.elapsed() << " ms";

    //time.restart();
    musicPlayer->stop();
    playlistTableView->setEnabled(false);
    playlistModel->removeRows(0, playlistModel->rowCount());
    qApp->processEvents();
    //qDebug() << Q_FUNC_INFO << "Cleaning used " << time.elapsed() << " ms";

    //time.restart();
    for (int i = 0; i < pluginLoader->musicSize(); ++i)
    {
        const MusicData& musicData = pluginLoader->musicData(i);
        int r = playlistModel->rowCount();
        playlistModel->insertRow(r);
        playlistModel->setMusicData(r, musicData);
        playlistModel->setLoop(r, 2);

        if ((i & 15) == 0)
            qApp->processEvents();
    }

    _loadSettingPlaylist();

    playlistTableView->setEnabled(true);
    //qDebug() << Q_FUNC_INFO << "Inserting used " << time.elapsed() << " ms";

    if (!playlistModel->rowCount())
        return;

    //playlistTableView->resizeRowsToContents();
    currentIndex = playlistTableView->verticalHeader()->logicalIndex(0);
    musicPlayer->setCurrentMusic(playlistModel->musicData(currentIndex), playlistModel->loop(currentIndex));

    nextAction->setEnabled(true);
    previousAction->setEnabled(true);
}

void MainWindow::saveFile()
{
    int id = playlistTableView->selectionModel()->currentIndex().row();
    static QString filter;
    static QString suffix = ".wav";
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save music file to..."),
        QString("%1.%2%3")
            .arg(playlistModel->musicData(id).trackNumber(), 2, 10, QLatin1Char('0'))
            .arg(playlistModel->musicData(id).title())
            .arg(suffix),
        MusicSaverFactory::filterStringList().join(";;"),
        &filter);
    if (!fileName.size())
        return;

    //qDebug() << Q_FUNC_INFO << QFileInfo(fileName).suffix();
    MusicSaver* musicSaver = MusicSaverFactory::createMusicSaver(filter);
    suffix = musicSaver->suffix();
    if (QFileInfo(fileName).suffix() != musicSaver->suffix().mid(1))
        fileName.append(musicSaver->suffix());
    if (!musicSaver->save(fileName, playlistModel->musicData(id), playlistModel->loop(id)))
        QMessageBox::warning(this, tr("Fatal Error"), musicSaver->errorString());
    delete musicSaver;
}

void MainWindow::about()
{
    playlistTableView->resizeRowsToContents();
    //qDebug() << Q_FUNC_INFO << playlistTableView->columnWidth(0) << playlistTableView->columnWidth(1) << playlistTableView->columnWidth(2);
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

void MainWindow::stateChanged(MusicPlayerState newState, MusicPlayerState /* oldState */)
{
    switch (newState)
    {
        case ErrorState:
            if (musicPlayer->errorType() == FatalError)
                QMessageBox::warning(this, tr("Fatal Error"), musicPlayer->errorString());
            else
                QMessageBox::warning(this, tr("Error"), musicPlayer->errorString());
            return;
        case PlayingState:
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            return;
        case StoppedState:
            stopAction->setEnabled(false);
            playAction->setEnabled(true);
            pauseAction->setEnabled(false);
            timeLcd->display("00:00.000");
            return;
        case PausedState:
            pauseAction->setEnabled(false);
            stopAction->setEnabled(true);
            playAction->setEnabled(true);
            return;
        default:
            return;
    }
}

void MainWindow::totalSamplesChanged(qint64 newTotalSamples)
{
    seekSlider->setMaximum(newTotalSamples);
    //qDebug() << Q_FUNC_INFO << newTotalSamples;
}

void MainWindow::loopChanged(uint newLoop)
{
    titleLabel->setText(QString("%1 %2").arg(playlistModel->musicData(currentIndex).title()).arg(RepeatString(musicPlayer->totalLoop() - newLoop)));
}

void MainWindow::tick(qint64 samples)
{
    int time = samples * (1.0 / 44.1) + 0.5;
    int msec = time % 1000;
    time /= 1000;
    int sec = time % 60;
    time /= 60;
    int min = time % 60;
    QTime displayTime(0, min, sec, msec);
    timeLcd->display(displayTime.toString("mm:ss.zzz"));
    seekSlider->setValue(samples);
}

void MainWindow::currentMusicChanged(const MusicData& musicData)
{
    currentIndex = playlistModel->indexOf(musicData);
    playlistTableView->selectRow(currentIndex);
    loopChanged(musicPlayer->loop());
    timeLcd->display("00:00.000");
}

int MainWindow::getNewId(int offset)
{
    offset %= playlistModel->rowCount();
    if (offset < 0)
        offset += playlistModel->rowCount();
    int ret = playlistTableView->verticalHeader()->visualIndex(currentIndex);
    do
    {
        ret = (ret+offset) % playlistModel->rowCount();
        if (ret == currentIndex)
            break;
    }
    while (playlistModel->loop(currentIndex) == 0);
    return playlistTableView->verticalHeader()->logicalIndex(ret);
}

void MainWindow::aboutToFinish()
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(playlistModel->rowCount() > currentIndex);
    int next = getNewId(1);
    musicPlayer->enqueue(playlistModel->musicData(next), playlistModel->loop(next));
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
    musicPlayer->stop();
    musicPlayer->clearQueue();

    musicPlayer->setCurrentMusic(playlistModel->musicData(row), playlistModel->loop(row));

    musicPlayer->play();
}

void MainWindow::playlistDoubleClicked(const QModelIndex& index)
{
    if (index.isValid() && index.column() != 0)
    {
        //qDebug() << Q_FUNC_INFO << index.row() << ',' << index.column();
        musicChanged(index.row(), index.column());
    }
}

void MainWindow::setupActions()
{
    playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("&Play"), this);
    playAction->setShortcut(tr("Ctrl+P"));
    playAction->setDisabled(true);
    pauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPause), tr("P&ause"), this);
    pauseAction->setShortcut(tr("Ctrl+A"));
    pauseAction->setDisabled(true);
    stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("S&top"), this);
    stopAction->setShortcut(tr("Ctrl+T"));
    stopAction->setDisabled(true);
    nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("&Next"), this);
    nextAction->setShortcut(tr("Ctrl+N"));
    nextAction->setDisabled(true);
    previousAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("P&revious"), this);
    previousAction->setShortcut(tr("Ctrl+R"));
    previousAction->setDisabled(true);
    loadFileAction = new QAction(tr("&Load data file"), this);
    loadFileAction->setShortcut(tr("Ctrl+L"));
    saveFileAction = new QAction(tr("&Save music file"), this);
    saveFileAction->setShortcut(tr("Ctrl+S"));
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
    connect(saveFileAction, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(configAction, SIGNAL(triggered()), this, SLOT(config()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(loadFileAction);
    fileMenu->addAction(saveFileAction);
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
    QToolBar *bar = new QToolBar(this);

    bar->addAction(playAction);
    bar->addAction(pauseAction);
    bar->addAction(stopAction);
    bar->addAction(previousAction);
    bar->addAction(nextAction);

    seekSlider = new QSlider(Qt::Horizontal, this);
    connect(musicPlayer, SIGNAL(totalSamplesChanged(qint64)), this, SLOT(totalSamplesChanged(qint64)));
    connect(seekSlider, SIGNAL(sliderMoved(int)), this, SLOT(seek(int)));

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setMaximum(128);
    volumeSlider->setValue(128);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
    connect(musicPlayer, SIGNAL(volumeChanged(qreal)), this, SLOT(setVolume(qreal)));

    titleLabel = new QLabel(this);

    QPalette palette;
    palette.setBrush(QPalette::Light, Qt::darkGray);

    timeLcd = new QLCDNumber(this);
    timeLcd->setPalette(palette);
    timeLcd->setNumDigits(9);
    timeLcd->display("00:00.000");

    playlistTableView = new QTableView(this);
    playlistTableView->setModel(playlistModel);
    playlistTableView->setItemDelegate(spinBoxDelegate);
    playlistTableView->setColumnWidth(0, 56);
    playlistTableView->setColumnWidth(1, 251);
    playlistTableView->setColumnWidth(2, 295);
    playlistTableView->setTabKeyNavigation(false);
    playlistTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistTableView->verticalHeader()->setMovable(true);
    playlistTableView->horizontalHeader()->setMovable(true);
    connect(playlistTableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(playlistDoubleClicked(const QModelIndex&)));

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
    mainLayout->addWidget(playlistTableView);

    QWidget *widget = new QWidget(this);
    widget->setLayout(mainLayout);

    setCentralWidget(widget);
    setWindowTitle(tr("Touhou Music Player"));
}

void MainWindow::_loadSettingLoadOnStartup()
{
    QSettings settings;

    settings.beginGroup(QLatin1String("General"));
    bool loadDataFileOnStartup = settings.value("Load On Startup").toBool();
    settings.endGroup();
    if (loadDataFileOnStartup)
        loadFile();
}

void MainWindow::_loadSettingPluginLoaderPath()
{
    QSettings settings;

    settings.beginGroup(QLatin1String("General"));
    int size = settings.beginReadArray("Applications Directory");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        QString title = settings.value("Title").toString();
        QString path = settings.value("Path").toString();
        loadingTitle = title;

        if (path.size() && pluginLoader->contains(title) && !pluginLoader->load(title, path))
            QMessageBox::warning(this, tr("Fatal Error"), tr("%1 is not the installation path of %2.").arg(path).arg(title));
    }
    settings.endArray();
    settings.endGroup();
}

void MainWindow::_loadSettingPlaylist()
{
    QSettings settings;
    settings.beginGroup("Playlist");
    int size = settings.beginReadArray("Items");
    for (int i = 0; i < size && i < playlistModel->rowCount(); ++i)
    {
        settings.setArrayIndex(i);
        int visualIndex = settings.value("Visual Index", i).toInt();
        int repeat = settings.value("Repeat", 2).toInt();
        playlistModel->setLoop(i, repeat);
        playlistTableView->verticalHeader()->swapSections(playlistTableView->verticalHeader()->visualIndex(i), visualIndex);
    }
    settings.endArray();
    settings.endGroup();
}
