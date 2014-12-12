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
#include <QSettings>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QFileDialog>

#include <QTemporaryFile>
#include <QProcess>

#include "ui_mainwindow.h"
#include "client.h"


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

  void on_toolButton_Previous_clicked();

  void on_toolButton_Stop_clicked();

  void on_toolButton_Pause_clicked();

  void on_toolButton_Play_clicked();

  void on_toolButton_Next_clicked();

  void on_toolButton_Upload_clicked();

  void on_pushButton_RefreshPortList_clicked();

  void on_pushButton_Connect_clicked();

  void on_pushButton_FormatSD_clicked();



  void handleDeviceStatusChanged(bool connected);

  void handleInfoStatusResponse(bool success, status_hdr_t* status, QList<fileheader_data_t>*fileList);

  void handleSendCommandResponse(bool success);

  void handleSendFileHeaderResponse(bool success);

  void handleSendFileChunkResponse(bool success, uint32_t chunk_id, uint32_t chunksCount);

  void handleSendFileTimeout();

  void handleBufferError(buffer_status_t bufferStatus);

  void handleStatusChanged(buffer_status_t bufferStatus);

  void 	handleFfmpegProcessStarted();

  void 	handleFfmpegProcessError(QProcess::ProcessError error);

  void 	handleFfmpegProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

  void 	handleFfmpegProcessReadyRead();

private:
  Ui::MainWindow *ui;
  Client *m_client;
  QTemporaryFile *m_tmpFile;
  QProcess *m_ffmpegProcess;
  QString m_shortFilename;
  QSettings* m_settings;

  void openSerialPort();

  void refreshSerialPortList();

  void loadBaudRateList();

  void loadSampleRateList();

  void updateConnectButtonLabel();

  void closeSerialPort();

  void log(QString msg, bool newLine=true);

};

#endif // MAINWINDOW_H
