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
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    mediaObject = new Phonon::MediaObject(this);

    mediaObject->setTickInterval(100);
    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            this, SLOT(stateChanged(Phonon::State, Phonon::State)));
    connect(mediaObject, SIGNAL(currentSourceChanged(const Phonon::MediaSource &)),
            this, SLOT(sourceChanged(const Phonon::MediaSource &)));
    connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

    Phonon::createPath(mediaObject, audioOutput);

    setupPlugins();
    setupActions();
    setupMenus();
    setupUi();
    timeLcd->display("00:00.000");
}

MainWindow::~MainWindow()
{
    delete mediaObject;
    mediaObject = NULL;
    delete audioOutput;
    audioOutput = NULL;
}

void MainWindow::setupPlugins()
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
            LoaderInterface* iLoader = qobject_cast<LoaderInterface *>(plugin);
            if (iLoader)
            {
                loaders_map.insert(iLoader->title(), loaders.size());
                loaders << iLoader;
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mediaObject->stop();
    mediaObject->clearQueue();
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
        if (loaders_map.contains(title))
        {
            int id = loaders_map.value(title);
            configDialog->setDir(id, path);
        }
    }
    settings.endArray();
    if (configDialog->exec() == QDialog::Accepted)
    {
        settings.beginWriteArray("appDir");
        for (int i = 0; i < loaders.size(); ++i)
        {
            settings.setArrayIndex(i);
            settings.setValue("title", loaders.at(i)->title());
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
    musicTable->resizeColumnsToContents();
    musicTable->resizeRowsToContents();
}

void MainWindow::loadFile()
{
    QSettings settings;

    int size = settings.beginReadArray("appDir");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        QString title = settings.value("title").toString();
        QString path = settings.value("path").toString();
        qDebug() << "title" << title;

        if (!loaders_map.contains(title))
            continue;

        LoaderInterface* dataLoader = loaders.at(loaders_map.value(title));
        if (!dataLoader->open(path))
        {
            QMessageBox::warning(this, tr("Fatal Error"), tr("%1 is not the installation path of %2.").arg(path).arg(title));
            continue;
        }
        for (uint i = 0; i < dataLoader->size(); ++i)
        {
            MusicData m(dataLoader->at(i));
            insertMusic(MusicFile(m, "music"));
        }
        dataLoader->close();
    }
    settings.endArray();

    if (musics.size())
    {
        musicTable->setEnabled(true);

        nextIndex = currentIndex = 0;
        qDebug() << "begin setCurrentSource";
        mediaObject->setCurrentSource(musics.at(0).source());
        qDebug() << "end setCurrentSource";

        nextAction->setEnabled(true);
        previousAction->setEnabled(true);
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Touhou Music Player"), tr(
        "Touhou Music Player 0.1 by BestSteve (ibmibmibm - ptt.cc)\n\n"
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
    ));
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
    switch (newState) {
        case Phonon::ErrorState:
            if (mediaObject->errorType() == Phonon::FatalError)
            {
                QMessageBox::warning(this, tr("Fatal Error"), mediaObject->errorString());
            }
            else
            {
                QMessageBox::warning(this, tr("Error"), mediaObject->errorString());
            }
            break;
        case Phonon::PlayingState:
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            break;
        case Phonon::StoppedState:
            stopAction->setEnabled(false);
            playAction->setEnabled(true);
            pauseAction->setEnabled(false);
            timeLcd->display("00:00.000");
            break;
        case Phonon::PausedState:
            pauseAction->setEnabled(false);
            stopAction->setEnabled(true);
            playAction->setEnabled(true);
            break;
        case Phonon::BufferingState:
            break;
        default:
            ;
    }
}

void MainWindow::tick(qint64 time)
{
    if (!musics.at(currentIndex).isLoop())
        goto out;
    if (currentLoop)
    {
        qint64 diff = time - musics.at(currentIndex).loopEnd();
        if (diff >= 0)
        {
            mediaObject->seek(musics.at(currentIndex).loopStart() + diff);
            --currentLoop;
            titleLabel->setText(QString("%1 %2").arg(musics.at(currentIndex).title()).arg(RepeatString(currentLoop)));
        }
        goto out;
    }
    if (audioOutput->volume() > 0.01)
    {
        audioOutput->setVolume(audioOutput->volume() - 0.01);
        goto out;
    }
    audioOutput->setVolume(1.0);
    aboutToFinish();
    musicChanged(nextIndex, 0);
    return;
out:
    int msec = time % 1000;
    time /= 1000;
    int sec = time % 60;
    time /= 60;
    int min = time % 60;
    QTime displayTime(0, min, sec, msec);
    timeLcd->display(displayTime.toString("mm:ss.zzz"));
}

void MainWindow::sourceChanged(const Phonon::MediaSource &/*source*/)
{
    if (currentIndex == nextIndex && !musics.at(currentIndex).isLoop() && (currentLoop > 1))
    {
        --currentLoop;
    }
    else
    {
        currentIndex = nextIndex;
        currentLoop = qobject_cast<QSpinBox*>(musicTable->cellWidget(currentIndex, 0))->value();
    }
    musicTable->selectRow(currentIndex);
    titleLabel->setText(QString("%1 %2").arg(musics.at(currentIndex).title()).arg(RepeatString(currentLoop)));
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
    if (!musics.at(currentIndex).isLoop() && (currentLoop > 1))
    {
        mediaObject->enqueue(musics.at(currentIndex).source());
        return;
    }
    int next = getNewId(1);
    mediaObject->enqueue(musics.at(next).source());
    nextIndex = next;
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
    bool wasPlaying = mediaObject->state() == Phonon::PlayingState;

    mediaObject->stop();
    mediaObject->clearQueue();

    nextIndex = row;
    mediaObject->setCurrentSource(musics.at(row).source());

    if (wasPlaying)
        mediaObject->play();
    else
        mediaObject->stop();
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

    connect(playAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
    connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
    connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
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
    configDialog = new ConfigDialog(loaders, this);

    QToolBar *bar = new QToolBar(this);

    bar->addAction(playAction);
    bar->addAction(pauseAction);
    bar->addAction(stopAction);
    bar->addAction(previousAction);
    bar->addAction(nextAction);

    seekSlider = new Phonon::SeekSlider(this);
    seekSlider->setMediaObject(mediaObject);

    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setAudioOutput(audioOutput);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

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

    musicTable->resizeColumnsToContents();
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
