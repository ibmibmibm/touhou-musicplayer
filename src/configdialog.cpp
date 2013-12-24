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
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QLabel>
#include <QSignalMapper>
#include <QStyle>
#include <QPushButton>
#include <QSettings>
#include <QtDebug>
#include "configdialog.h"

GeneralConfigTab::GeneralConfigTab(int pluginCount_, QWidget *parent) :
    QWidget(parent),
    pluginCount(pluginCount_)
{
    QGroupBox *dirGroupBox = new QGroupBox(tr("Programs' Directory"));

    dirLayout = new QGridLayout();

    {
        QLabel* label = new QLabel(tr("Name"));
        label->setAlignment(Qt::AlignCenter);
        dirLayout->addWidget(label, 0, 0, Qt::AlignCenter);
        label = new QLabel(tr("Path"));
        label->setAlignment(Qt::AlignCenter);
        dirLayout->addWidget(label, 0, 1, 1, 2, Qt::AlignCenter);
    }

    QSignalMapper *signalMapper = new QSignalMapper(this);
    for (int i = 0; i < pluginCount; ++i)
    {
        QPushButton *fileButton = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), "");
        connect(fileButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
        signalMapper->setMapping(fileButton, i);

        dirLayout->addWidget(new QLabel(), i + 1, 0);
        dirLayout->addWidget(new QLineEdit(), i + 1, 1);
        dirLayout->addWidget(fileButton, i + 1, 2);
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(chooseDir(int)));

    dirGroupBox->setLayout(dirLayout);

    loadCheckBox = new QCheckBox(tr("Load data files on startup."));

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(dirGroupBox);
    mainLayout->addWidget(loadCheckBox);
    mainLayout->addStretch(1);

    this->setLayout(mainLayout);
}

void GeneralConfigTab::chooseDir(int i)
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose the installation path of the program..."));
    if (path.size() != 0)
    {
        setPluginDir(i, path);
    }
}

bool GeneralConfigTab::checkValues()
{
    for (int i = 0; i < pluginCount; ++i)
    {
        QLineEdit* lineEdit(qobject_cast<QLineEdit*>(dirLayout->itemAtPosition(i + 1, 1)->widget()));
        const QString& dir(lineEdit->text());
        if (dir.size() && !QDir(dir).exists())
        {
            QMessageBox::critical(this, tr("Fatal Error"), tr("This directory does not exist: %1.").arg(dir));
            lineEdit->setFocus();
            return false;
        }
    }
    return true;
}



PlaybackConfigTab::PlaybackConfigTab(QWidget *parent) :
    QWidget(parent)
{
    deviceComboBox = new QComboBox();
    deviceComboBox->setEditable(false);

    QHBoxLayout *bufferLayout = new QHBoxLayout();
    bufferLayout->addWidget(new QLabel(tr("Output Device")));
    bufferLayout->addWidget(deviceComboBox, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(bufferLayout);
    mainLayout->addStretch(1);

    this->setLayout(mainLayout);
}

bool PlaybackConfigTab::checkValues()
{
    return true;
}



ConfigDialog::ConfigDialog(const PluginLoader *const _pluginLoader, const MusicPlayer *const _musicPlayer, QWidget *parent) :
    QDialog(parent),
    pluginLoader(_pluginLoader),
    musicPlayer(_musicPlayer)
{
    setupUi();
    loadSettings();
}

void ConfigDialog::accept()
{
    //qDebug() << Q_FUNC_INFO;
    if (!generalConfigTab->checkValues())
    {
        tabWidget->setCurrentWidget(generalConfigTab);
        return;
    }
    if (!playbackConfigTab->checkValues())
    {
        tabWidget->setCurrentWidget(playbackConfigTab);
        return;
    }
    saveSettings();
    QDialog::accept();
}

void ConfigDialog::loadSettings()
{
    QSettings settings;
    settings.beginGroup("General");
    int size = settings.beginReadArray("Applications Directory");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        QString title = settings.value("Title").toString();
        QString path = settings.value("Path").toString();
        if (pluginLoader->contains(title))
        {
            int id = pluginLoader->id(title);
            generalConfigTab->setPluginDir(id, path);
        }
    }
    settings.endArray();
    generalConfigTab->setLoadOnStartup(settings.value("Load On Startup").toBool());
    settings.endGroup();

    settings.beginGroup("Playback");
    playbackConfigTab->setDevice(settings.value("Output Device", static_cast<int>(musicPlayer->defaultDevice())).toInt());
    settings.endGroup();
}

void ConfigDialog::saveSettings()
{
    QSettings settings;
    settings.beginGroup("General");
    settings.beginWriteArray("Applications Directory");
    for (int i = 0; i < pluginLoader->size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("Title", pluginLoader->title(i));
        settings.setValue("Path", generalConfigTab->pluginDir(i));
    }
    settings.endArray();
    settings.setValue("Load On Startup", generalConfigTab->loadOnStartup());
    settings.endGroup();

    settings.beginGroup("Playback");
    settings.setValue("Output Device", playbackConfigTab->device());
    settings.endGroup();
}

void ConfigDialog::setupUi()
{
    generalConfigTab = new GeneralConfigTab(pluginLoader->size());
    for (int i = 0; i < pluginLoader->size(); ++i)
        generalConfigTab->setPluginTitle(i, pluginLoader->title(i));

    playbackConfigTab = new PlaybackConfigTab();
    for (int i = 0; i < musicPlayer->deviceCount(); ++i)
        playbackConfigTab->addDevice(musicPlayer->device(i));

    tabWidget = new QTabWidget();
    tabWidget->addTab(generalConfigTab, tr("General"));
    tabWidget->addTab(playbackConfigTab, tr("Playback"));

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(tabWidget, 1);
    mainLayout->addWidget(buttonBox);

    this->setLayout(mainLayout);

    setWindowTitle(tr("Config"));
}
