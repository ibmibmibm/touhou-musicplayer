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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QList>
#include <QSize>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include "pluginloader.h"
#include "musicplayer.h"

class GeneralConfigTab : public QWidget
{
    Q_OBJECT
    public:
        GeneralConfigTab(int pluginCount_, QWidget *parent = 0);
        QString pluginTitle(int id) const { return qobject_cast<QLabel *>(dirLayout->itemAtPosition(id + 1, 0)->widget())->text(); }
        void setPluginTitle(int id, const QString& dir) { qobject_cast<QLabel *>(dirLayout->itemAtPosition(id + 1, 0)->widget())->setText(dir); }
        QString pluginDir(int id) const { return qobject_cast<QLineEdit *>(dirLayout->itemAtPosition(id + 1, 1)->widget())->text(); }
        void setPluginDir(int id, const QString& dir) { qobject_cast<QLineEdit *>(dirLayout->itemAtPosition(id + 1, 1)->widget())->setText(dir); }
        bool loadOnStartup() const { return loadCheckBox->isChecked(); }
        void setLoadOnStartup(bool value) { return loadCheckBox->setChecked(value); }
        bool checkValues();
    public slots:
        void chooseDir(int id);
    private:
        int pluginCount;
        QGridLayout* dirLayout;
        QCheckBox* loadCheckBox;
};

class PlaybackConfigTab : public QWidget
{
    Q_OBJECT
    public:
        PlaybackConfigTab(QWidget *parent = 0);
        void addDevice(const QString& name) { deviceComboBox->addItem(name); }
        int device() const { return deviceComboBox->currentIndex(); }
        void setDevice(int id) { deviceComboBox->setCurrentIndex(id); }
        bool checkValues();
    private:
        QComboBox* deviceComboBox;
};

class ConfigDialog : public QDialog
{
    Q_OBJECT

    public:
        ConfigDialog(const PluginLoader *const _pluginLoader, const MusicPlayer *const _musicPlayer, QWidget *parent = 0);
        ~ConfigDialog() {}

        QSize sizeHint() const {
            return QSize(600, 300);
        }

    public slots:
        virtual void accept();

    private:
        void setupUi();
        void loadSettings();
        void saveSettings();

        const PluginLoader *const pluginLoader;
        const MusicPlayer *const musicPlayer;
        GeneralConfigTab* generalConfigTab;
        PlaybackConfigTab* playbackConfigTab;
        QTabWidget* tabWidget;
        QDialogButtonBox* buttonBox;
};

#endif //CONFIGDIALOG_H
