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
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QLabel>
#include <QSignalMapper>
#include <QStyle>
#include <QPushButton>
#include "configdialog.h"

ConfigDialog::ConfigDialog(const QList<QString>& _plugin_title, QWidget *parent) :
    QDialog(parent),
    plugin_title()
{
    /* workaround */
    for (int i = 0; i < _plugin_title.size(); ++i)
    {
        plugin_title << _plugin_title.at(i);
    }
    setupUi();
}

const QString ConfigDialog::dir(int i) const
{
    return qobject_cast<QLineEdit *>(dirLayout->itemAtPosition(i + 1, 1)->widget())->text();
}

void ConfigDialog::setDir(int i, const QString& path)
{
    qobject_cast<QLineEdit *>(dirLayout->itemAtPosition(i + 1, 1)->widget())->setText(path);
}

void ConfigDialog::chooseDir(int i)
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose the installation path of the program..."));
    if (path.size())
    {
        setDir(i, path);
    }
}

void ConfigDialog::accept()
{
    //qDebug() << Q_FUNC_INFO;
    for (int i = 0; i < plugin_title.size(); ++i)
    {
        QLineEdit* lineEdit = qobject_cast<QLineEdit *>(dirLayout->itemAtPosition(i + 1, 1)->widget());
        Q_ASSERT(lineEdit != NULL);
        if (lineEdit->text().size() && !QDir(lineEdit->text()).exists())
        {
            QMessageBox::critical(this, tr("Fatal Error"), tr("This directory does not exist: %1.").arg(lineEdit->text()));
            lineEdit->setFocus();
            return;
        }
    }
    QDialog::accept();
}

void ConfigDialog::setupUi()
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
    for (int i = 0; i < plugin_title.size(); ++i)
    {
        QPushButton *fileButton = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), "");
        connect(fileButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
        signalMapper->setMapping(fileButton, i);

        dirLayout->addWidget(new QLabel(plugin_title.at(i)), i + 1, 0);
        dirLayout->addWidget(new QLineEdit(), i + 1, 1);
        dirLayout->addWidget(fileButton, i + 1, 2);
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(chooseDir(int)));

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    dirGroupBox->setLayout(dirLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(dirGroupBox);
    mainLayout->addStretch(1);
    mainLayout->addWidget(buttonBox);

    this->setLayout(mainLayout);

    setWindowTitle(tr("Config"));
}
