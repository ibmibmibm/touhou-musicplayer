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
#include <QMessageBox>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QFileDialog>

#include "mainwindow.h"
#include "musicsaver_wav.h"

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
    connect(musicPlayer, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(musicPlayer, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
    connect(musicPlayer, SIGNAL(currentMusicChanged(const MusicData&)), this, SLOT(currentMusicChanged(const MusicData&)));
    connect(musicPlayer, SIGNAL(loopChanged(uint)), this, SLOT(loopChanged(uint)));

    setupActions();
    setupMenus();
    setupUi();
    timeLcd->display("00:00.000");
    {
        QSettings settings;

        settings.beginGroup(QLatin1String("General"));
        bool loadDataFileOnStartup = settings.value("Load On Startup").toBool();
        settings.endGroup();
        if (loadDataFileOnStartup)
            loadFile();
    }
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
    ConfigDialog(pluginLoader, musicPlayer, this).exec();
}

void MainWindow::insertMusic(const MusicData& musicData)
{
    musicDataList << musicData;

    int r = musicTable->rowCount();
    musicTable->insertRow(r);

    QSpinBox *spin = new QSpinBox(this);
    musicTable->setCellWidget(r, 0, spin);
    spin->setValue(2);

    QTableWidgetItem *titleItem = new QTableWidgetItem(musicData.title());
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
    musicTable->setItem(r, 1, titleItem);

    QTableWidgetItem *albumItem = new QTableWidgetItem(musicData.album());
    albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);
    musicTable->setItem(r, 2, albumItem);
}

void MainWindow::loadFile()
{
    pluginLoader->clear();

    QTime time;
    time.restart();
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
    qDebug() << "Loading used " << time.elapsed() << " ms";

    time.restart();
    musicPlayer->stop();
    musicDataList.clear();
    musicTable->setEnabled(false);
    musicTable->setRowCount(0);
    qApp->processEvents();
    qDebug() << "Cleaning used " << time.elapsed() << " ms";

    time.restart();
    for (int i = 0; i < pluginLoader->musicSize(); ++i)
    {
        insertMusic(pluginLoader->musicData(i));
        if ((i & 15) == 0)
            qApp->processEvents();
    }
    musicTable->setEnabled(true);
    qDebug() << "Inserting used " << time.elapsed() << " ms";

    if (musicDataList.size())
    {
        musicTable->resizeRowsToContents();
        currentIndex = 0;
        musicPlayer->setCurrentMusic(musicDataList.at(0), qobject_cast<QSpinBox*>(musicTable->cellWidget(0, 0))->value());

        nextAction->setEnabled(true);
        previousAction->setEnabled(true);
    }
}

void MainWindow::saveFile()
{
    QString filter;
    QStringList filterList;
    filterList << MusicSaver_Wav::filterString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save music file to..."), QString(), filterList.join(";;"), &filter);
    if (!fileName.size())
        return;

    int id = musicTable->currentRow();
    quint64 fadeoutTime;
    {
        QSettings settings;
        settings.beginGroup("Playback");
        fadeoutTime = settings.value("Fadeout Time", 10000U).toUInt();
        settings.endGroup();
    }
    MusicSaver* musicSaver = new MusicSaver_Wav;
    if (!musicSaver->save(fileName, musicDataList.at(id), qobject_cast<QSpinBox*>(musicTable->cellWidget(id, 0))->value(), fadeoutTime))
        QMessageBox::warning(this, tr("Fatal Error"), musicSaver->errorString());
    delete musicSaver;
}

void MainWindow::about()
{
    musicTable->resizeRowsToContents();
    //qDebug() << musicTable->columnWidth(0) << musicTable->columnWidth(1) << musicTable->columnWidth(2);
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

void MainWindow::totalSamplesChanged(qint64 newTotalSamples)
{
    seekSlider->setMaximum(newTotalSamples);
    //qDebug() << newTotalSamples;
}

void MainWindow::loopChanged(uint newLoop)
{
    titleLabel->setText(QString("%1 %2").arg(musicDataList.at(currentIndex).title()).arg(RepeatString(musicPlayer->totalLoop() - newLoop)));
}

void MainWindow::tick(qint64 samples)
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

void MainWindow::currentMusicChanged(const MusicData& musicData)
{
    currentIndex = musicDataList.indexOf(musicData);
    musicTable->selectRow(currentIndex);
    titleLabel->setText(QString("%1 %2").arg(musicDataList.at(currentIndex).title()).arg(RepeatString(musicPlayer->remainLoop())));
    timeLcd->display("00:00.000");
}

int MainWindow::getNewId(int offset)
{
    offset %= musicDataList.size();
    if (offset < 0)
        offset += musicDataList.size();
    int ret = currentIndex;
    do
    {
        ret = (ret+offset) % musicDataList.size();
        if (ret == currentIndex)
            break;
    }
    while (qobject_cast<QSpinBox*>(musicTable->cellWidget(ret, 0))->value() == 0);
    return ret;
}

void MainWindow::aboutToFinish()
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(musicDataList.size() > currentIndex);
    int next = getNewId(1);
    musicPlayer->enqueue(musicDataList.at(next), qobject_cast<QSpinBox*>(musicTable->cellWidget(next, 0))->value());
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

    musicPlayer->setCurrentMusic(musicDataList.at(row), qobject_cast<QSpinBox*>(musicTable->cellWidget(row, 0))->value());
    titleLabel->setText(QString("%1 %2").arg(musicDataList.at(currentIndex).title()).arg(RepeatString(musicPlayer->remainLoop())));

    musicPlayer->play();
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

    {
        QStringList title;
        title << tr("Repeat") << tr("Title") << tr("Album");
        musicTable = new QTableWidget(0, title.size(), this);
        musicTable->setHorizontalHeaderLabels(title);
        musicTable->setColumnWidth(0, 56);
        musicTable->setColumnWidth(1, 251);
        musicTable->setColumnWidth(2, 295);
    }
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(musicTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(musicChanged(int, int)));

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
