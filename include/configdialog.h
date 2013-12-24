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
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QtDebug>

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(const QList<QString>& plugin_title, QWidget *parent = 0);
    ~ConfigDialog() {}

    QSize sizeHint() const {
        return QSize(600, 300);
    }

    const QString dir(int i) const;
    void setDir(int i, const QString& dir);

public slots:
    void chooseDir(int i);
    virtual void accept();

private:
    void setupUi();

    QGridLayout* dirLayout;
    QDialogButtonBox* buttonBox;
    const QList<QString> plugin_title;
    int count;
};

#endif //CONFIGDIALOG_H
