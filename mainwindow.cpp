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

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    ui->dockWidget_terminal->setWidget(console);
    serial = new QSerialPort(this);
    client = new Client(this);
    settings = new SettingsDialog;


    m_format.setSampleRate(11025);
    m_format.setChannelCount(1);
    m_format.setSampleSize(8);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::UnSignedInt);

    m_buffer = new QBuffer(&m_buffer_data);
    m_buffer->open(QIODevice::ReadWrite);

    m_audio = new QAudioOutput(m_format, this);
    m_audio->setBufferSize(4096);
    connect(m_audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    m_audio->start(m_buffer);


    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);

    initActionsConnections();

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(console, SIGNAL(getData(QByteArray)), this, SLOT(writeData(QByteArray)));

    connect(serial, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));

    ui->progressBar->setValue(0);

}

MainWindow::~MainWindow()
{
    delete settings;
    delete ui;
}

void MainWindow::onBytesWritten(qint64 bytes)
{
  Q_UNUSED(bytes);
  if(m_file.bytesAvailable()>1024){
    serial->write(m_file.read(1024));
  }
  float progress = 1.0f - float(m_file.bytesAvailable()) / float(m_file.size());
  ui->progressBar->setValue((int) qRound(progress*100.0f));


}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    openSerialPort(p);
}


void MainWindow::openSerialPort(SettingsDialog::Settings p)
{
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
            console->setEnabled(true);
            console->setLocalEchoEnabled(p.localEchoEnabled);
            ui->actionConnect->setEnabled(false);
            ui->actionDisconnect->setEnabled(true);
            ui->actionConfigure->setEnabled(false);
            ui->statusBar->showMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                                       .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                       .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
        ui->statusBar->showMessage(tr("Open error"));
    }
}

void MainWindow::openSerialPort(QString port)
{
  SettingsDialog::Settings p = settings->settings();
  p.name = port;
  openSerialPort(p);

}


void MainWindow::closeSerialPort()
{
    serial->close();
    console->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    ui->statusBar->showMessage(tr("Disconnected"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Simple Terminal"),
                       tr("The <b>Simple Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}

void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}

void MainWindow::readData()
{
    QByteArray data = serial->readAll();
    console->putData(data);
    m_buffer_data.append(data);

}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(openSerialPort()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(closeSerialPort()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionConfigure, SIGNAL(triggered()), settings, SLOT(show()));
    connect(ui->actionClear, SIGNAL(triggered()), console, SLOT(clear()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionSelectFile, SIGNAL(triggered()), this, SLOT(selectFile()));
}

void MainWindow::selectFile()
{
    //QString fileName = QFileDialog::getOpenFileName( this,tr("Open Image"), "", tr("WAV Files (*.wav)"));
    //qDebug() << fileName;
}


void MainWindow::on_pushButton_filePicker_clicked()
{
  QString filename = "/home/diego/music/8bit.wav";
  m_file.setFileName(filename);
  m_file.open(QIODevice::ReadOnly);
  serial->write(m_file.read(1024));
}

void MainWindow::on_pushButton_openPortA_clicked()
{
  client->sendTest();
  //openSerialPort("/dev/pts/1");

}


void MainWindow::on_pushButton_openPortB_clicked()
{
  openSerialPort("/dev/pts/3");

}


void MainWindow::handleStateChanged(QAudio::State newState)
{
  qDebug() << "AudioSender::handleStateChanged: " << newState ;

  switch (newState) {
    case QAudio::IdleState:
      m_buffer->seek(0);
      // Finished playing (no more data)
      break;

    case QAudio::StoppedState:
      // Stopped for other reasons
      if (m_audio->error() != QAudio::NoError) {
        // Error handling
        qDebug() << "m_audio->error(): " << m_audio->error();

      }
      break;

    default:
      // ... other cases as appropriate
      break;
  }
}

void MainWindow::on_pushButton_playWavLocally_clicked()
{
  QString filename = "/home/diego/music/8bit.wav";
  m_file.setFileName(filename);
  m_file.open(QIODevice::ReadOnly);
  QByteArray data = m_file.read(sizeof(wav_hdr_t));
  qDebug() << data[0];
  wav_hdr_t* wav_header = (wav_hdr_t*) data.data();
  qDebug() << wav_header->ChunkID;
}

