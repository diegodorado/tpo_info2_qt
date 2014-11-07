/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QtGlobal>

#include <QMainWindow>

#include <QtSerialPort/QSerialPort>

#include "wav.h"
#include "audio_sender.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "client.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QBuffer>

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
  Q_OBJECT





public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
  void onBytesWritten(qint64 bytes);
  void openSerialPort();
  void closeSerialPort();
  void about();
  void writeData(const QByteArray &data);
  void readData();
  void selectFile();
  void handleError(QSerialPort::SerialPortError error);
  void on_pushButton_filePicker_clicked();
  void on_pushButton_openPortA_clicked();
  void on_pushButton_openPortB_clicked();
  void handleStateChanged(QAudio::State newState);
  void on_pushButton_playWavLocally_clicked();


private:
  Ui::MainWindow *ui;
  Console *console;
  QFile m_file;
  QAudioFormat m_format;
  QAudioOutput* m_audio;
  QByteArray m_buffer_data;
  QBuffer* m_buffer;
  SettingsDialog *settings;
  QSerialPort *serial;
  Client *client;

  void openSerialPort(SettingsDialog::Settings p);
  void openSerialPort(QString port);
  void initActionsConnections();

};

#endif // MAINWINDOW_H
