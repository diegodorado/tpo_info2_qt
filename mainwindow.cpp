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
    ui->groupBox_DeviceControl->setEnabled(false);
    ui->groupBox_AudioProgress->setEnabled(false);

    m_client = new Client(this);
    m_ffmpegProcess = new QProcess(this);

    m_settings = new QSettings("Grupo 4", "TPO Info 2");

    connect(m_client->getSerialPort(), SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleSerialError(QSerialPort::SerialPortError)));


    connect(m_client, SIGNAL(deviceStatusChanged(bool)), this, SLOT(handleDeviceStatusChanged(bool)));
    connect(m_client, SIGNAL(infoStatusResponse(bool, status_hdr_t*,QList<fileheader_data_t>*)),SLOT(handleInfoStatusResponse(bool , status_hdr_t*,QList<fileheader_data_t>*)));
    connect(m_client, SIGNAL(sendFileHeaderResponse(bool)), this, SLOT(handleSendFileHeaderResponse(bool)));
    connect(m_client, SIGNAL(sendFileChunkResponse(bool,uint32_t, uint32_t)), this, SLOT(handleSendFileChunkResponse(bool,uint32_t, uint32_t)));
    connect(m_client, SIGNAL(sendCommandResponse(bool)), this, SLOT(handleSendCommandResponse(bool)));
    connect(m_client, SIGNAL(log(QString)),SLOT(handleClientLog(QString)));



    connect(m_ffmpegProcess, SIGNAL(started()),SLOT(handleFfmpegProcessStarted()));
    connect(m_ffmpegProcess, SIGNAL(error(QProcess::ProcessError )),SLOT(handleFfmpegProcessError(QProcess::ProcessError )));
    connect(m_ffmpegProcess, SIGNAL(finished(int , QProcess::ExitStatus )),SLOT(handleFfmpegProcessFinished(int , QProcess::ExitStatus )));
    connect(m_ffmpegProcess, SIGNAL(readyRead()),SLOT(handleFfmpegProcessReadyRead()));
    connect(m_ffmpegProcess, SIGNAL(readyReadStandardError()),SLOT(handleFfmpegProcessReadyRead()));
    connect(m_ffmpegProcess, SIGNAL(readyReadStandardOutput()),SLOT(handleFfmpegProcessReadyRead()));



    ui->progressBar->setValue(0);

    refreshSerialPortList();
    loadSampleRateList();
    loadBaudRateList();

}

MainWindow::~MainWindow()
{
  delete ui;
  delete m_client;
}


void MainWindow::handleSerialError(QSerialPort::SerialPortError error)
{
  if(error ==QSerialPort::NoError)
    return;

  log(QString("Error Critico en puerto serie: %1").arg(m_client->getSerialPort()->errorString()));
  closeSerialPort();
}

void MainWindow::on_toolButton_Previous_clicked()
{
  log(QString("Enviando Comando 'Previous' ..."));
  m_client->sendCommandRequest(COMMAND_PREVIOUS);
}

void MainWindow::on_toolButton_Stop_clicked()
{
  log(QString("Enviando Comando 'Stop' ..."));
  m_client->sendCommandRequest(COMMAND_STOP);

}

void MainWindow::on_toolButton_Pause_clicked()
{
  log(QString("Enviando Comando 'Pause' ..."));
  m_client->sendCommandRequest(COMMAND_PAUSE);
}

void MainWindow::on_toolButton_Play_clicked()
{
  log(QString("Enviando Comando 'Play' ..."));
  m_client->sendCommandRequest(COMMAND_PLAY);
}

void MainWindow::on_toolButton_Next_clicked()
{
  log(QString("Enviando Comando 'Next' ..."));
  m_client->sendCommandRequest(COMMAND_NEXT);
}


void MainWindow::on_toolButton_Upload_clicked()
{

  QString program = "ffmpeg";
  QStringList arguments;
  QString filename;

  filename = QFileDialog::getOpenFileName( this,"Seleccionar Archivo de Audio", "", "Archivo de Audio (*.wav *.mp3)");
  QFileInfo fileInfo(filename);
  m_shortFilename = fileInfo.fileName().toUpper();
  m_tmpFile = new QTemporaryFile(this);

  if (filename != "")
  {

    if (m_tmpFile->open()) {
      // this is only to get a valid tmp filename

      m_settings->setValue("sample-rate",ui->comboBox_SampleRate->currentData().toInt() );

      // contruye el comando: ffmpeg -i source -ac 1 -sample_fmt u8 -acodec pcm_u8 -f u8 -y -ar 8000 /tmp.file

      arguments << "-i" << filename;
      arguments << "-ac" << "1"; // audo channels: mono
      arguments << "-sample_fmt" << "u8"; //8 bit sample depth
      arguments << "-acodec" << "pcm_u8"; // audio codec: pcm 8 bit
      arguments << "-f" << "u8"; // format is PCM... headless WAV
      arguments << "-y"; //overwrite if file exists... it will exists
      arguments << "-ar" <<  ui->comboBox_SampleRate->currentData().toString(); // audio sample rate
      arguments << m_tmpFile->fileName();

      log(QString("Ejecutando: %1 %2").arg(program).arg(arguments.join(" ")));
      m_ffmpegProcess->setProcessChannelMode(QProcess::MergedChannels);
      m_ffmpegProcess->start(program, arguments);

      ui->groupBox_DeviceControl->setEnabled(false);
      ui->groupBox_AudioProgress->setEnabled(true);

    }

  }
}

void MainWindow::on_pushButton_RefreshPortList_clicked()
{
  log(QString("Actualiza lista de puertos serie."));
  refreshSerialPortList();
}

void MainWindow::on_pushButton_Connect_clicked()
{
  if (m_client->getSerialPort()->isOpen())
    closeSerialPort();
  else
    openSerialPort();

}




void MainWindow::refreshSerialPortList()
{
  ui->comboBox_PortList->clear();
  foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
    ui->comboBox_PortList->addItem(info.portName(), info.portName());
  }
}

void MainWindow::loadBaudRateList()
{
  ui->comboBox_BaudRate->clear();

  ui->comboBox_BaudRate->addItem("9600", QSerialPort::Baud9600);
  ui->comboBox_BaudRate->addItem("19200",QSerialPort::Baud19200);
  ui->comboBox_BaudRate->addItem("38400",QSerialPort::Baud38400);
  ui->comboBox_BaudRate->addItem("57600",QSerialPort::Baud57600);
  ui->comboBox_BaudRate->addItem("115200",QSerialPort::Baud115200);

  if(m_settings->contains("baud-rate"))
    ui->comboBox_BaudRate->setCurrentIndex(ui->comboBox_BaudRate->findData(m_settings->value("baud-rate").toInt()));

}

void MainWindow::loadSampleRateList()
{
  ui->comboBox_SampleRate->clear();
  ui->comboBox_SampleRate->addItem("8 Khz", 8000);
  ui->comboBox_SampleRate->addItem("11 Khz", 11025);
  ui->comboBox_SampleRate->addItem("22 Khz", 22050);
  ui->comboBox_SampleRate->addItem("44 Khz", 44100);

  if(m_settings->contains("sample-rate"))
    ui->comboBox_SampleRate->setCurrentIndex(ui->comboBox_SampleRate->findData(m_settings->value("sample-rate").toInt()));

}




void MainWindow::updateConnectButtonLabel()
{
  if(m_client->getSerialPort()->isOpen())
  {
    ui->comboBox_PortList->setEnabled(false);
    ui->comboBox_BaudRate->setEnabled(false);
    ui->pushButton_RefreshPortList->setEnabled(false);
    ui->pushButton_Connect->setText("Desconectar");
  }
  else
  {
    ui->comboBox_PortList->setEnabled(true);
    ui->comboBox_BaudRate->setEnabled(true);
    ui->pushButton_RefreshPortList->setEnabled(true);
    ui->pushButton_Connect->setText("Conectar");
  }
}


void MainWindow::openSerialPort()
{
  QString port = ui->comboBox_PortList->currentData().toString();
  uint16_t baudRate = ui->comboBox_BaudRate->currentData().toInt();
  //save settings for next time
  m_settings->setValue("baud-rate",baudRate );

  log(QString("Intentando abrir puerto serie."));

  if (ui->comboBox_PortList->currentData().isNull()){
    ui->statusBar->showMessage(tr("Seleccione un puerto valido"));
    log(QString("El puerto no es válido."));
    return;
  }


  if(m_client->openSerialPort(port, baudRate))
  {
    ui->statusBar->showMessage("Conectado a " + ui->comboBox_PortList->currentData().toString());
    log(QString("Conectado a %1 .").arg(ui->comboBox_PortList->currentText()));
  }
  else
  {
    ui->statusBar->showMessage(tr("Error al abrir puerto"));
    log(QString("Error al abrir puerto %1 .").arg(ui->comboBox_PortList->currentText()));
  }

  updateConnectButtonLabel();

}

void MainWindow::closeSerialPort()
{

  log(QString("Cerrando puerto serie."));
  m_client->closeSerialPort();
  ui->listWidget_DeviceAudios->clear();
  ui->groupBox_DeviceControl->setEnabled(false);
  ui->statusBar->showMessage("No Conectado");

  updateConnectButtonLabel();

}

void MainWindow::log(QString msg, bool newLine )
{
  if(newLine)
    msg.append("\n");
  ui->plainTextEdit_Log->insertPlainText(msg);
  ui->plainTextEdit_Log->centerCursor();
}

void MainWindow::handleDeviceStatusChanged(bool connected)
{
  if(connected)
  {
    log(QString("Dispositivo conectado."));
    m_client->getDeviceStatus();
    log(QString("Solicitando estado del dispositivo..."));
  }
  else
  {
    log(QString("Dispositivo no detectado."));
    ui->listWidget_DeviceAudios->clear();
    ui->groupBox_DeviceControl->setEnabled(false);
    ui->groupBox_AudioProgress->setEnabled(false);
    ui->progressBar->setValue(0);

  }


}

void MainWindow::handleInfoStatusResponse(bool success, status_hdr_t* status, QList<fileheader_data_t> * fileList)
{
  ui->listWidget_DeviceAudios->clear();
  ui->groupBox_DeviceControl->setEnabled(success);

  if(success)
  {
    log(QString("Estado del dispositivo recibida."));
    log(QString(" --> blocks_count: %1.").arg(status->blocks_count));
    log(QString(" --> last_block: %1.").arg(status->last_block));
    log(QString(" --> Cantidad de Audios: %1.").arg(status->files_count));

    foreach (const fileheader_data_t &file_header, *fileList) {
      QString filename = QString::fromLatin1(file_header.filename,8);
      ui->listWidget_DeviceAudios->addItem(filename);
    }

  }
  else
  {
    log(QString("Error al recibir el estado del dispositivo."));
  }

}

void MainWindow::handleSendCommandResponse(bool success)
{
  if(success)
  {
    log(QString("Comando Aceptado."));
  }
  else
  {
    log(QString("Comando Rechazado."));
  }

}

void MainWindow::handleSendFileHeaderResponse(bool success)
{
  if(success)
  {
    log(QString("Envio de Audio Aceptado."));
  }
  else
  {
    log(QString("Envio de Audio Rechazado."));
    ui->groupBox_DeviceControl->setEnabled(true);
    ui->groupBox_AudioProgress->setEnabled(false);

  }
}

void MainWindow::handleSendFileChunkResponse(bool success, uint32_t chunk_id, uint32_t chunksCount)
{
  if(success)
  {
    log(QString("Chunk %1 / %2 recibido con éxito.").arg(chunk_id).arg(chunksCount));

    //fixme: not accurate at all!

    float progress = float(chunk_id) / float(chunksCount-1);
    ui->progressBar->setValue((int) qRound(progress*100.0f));

    if(chunk_id==chunksCount-1)
    {
      //todo: send a confirmation request...
      log(QString("Ultimo chunk de archivo recibido."));
      m_client->getDeviceStatus();
      log(QString("Solicitando estado del dispositivo..."));

      ui->groupBox_DeviceControl->setEnabled(true);
      ui->groupBox_AudioProgress->setEnabled(false);
    }

  }
  else
  {
    log(QString("Fallo la recepción de chunk %1 .").arg(chunk_id));
    ui->groupBox_DeviceControl->setEnabled(true);
    ui->groupBox_AudioProgress->setEnabled(false);

  }
}



void MainWindow::handleFfmpegProcessStarted()
{
  log(QString("ffmpeg process started."));

}

void MainWindow::handleFfmpegProcessError(QProcess::ProcessError error)
{
  log(QString("ffmpeg Process Error: %1").arg(error));

}

void MainWindow::handleFfmpegProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  log(QString("ffmpeg Process Finished. Exit code: %1 . Exit status: %2").arg(exitCode).arg(exitStatus));
  if(exitCode==0 && exitStatus==0)
  {
    log(QString("Conversion finalizada correctamente. Enviando audio..."));
    m_client->sendFile(m_tmpFile, ui->comboBox_SampleRate->currentData().toInt(), m_shortFilename);
  }
  else
  {
    log(QString("Conversion finalizada con errores."));
  }

}



void MainWindow::handleFfmpegProcessReadyRead()
{
  log(m_ffmpegProcess->readAll(), false);
}

void MainWindow::handleClientLog(QString message)
{
  log(message);
}






