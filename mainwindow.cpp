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
    //ui->groupBox_DeviceControl->setEnabled(false);
    ui->groupBox_AudioProgress->setEnabled(false);

    m_serialPort = new QSerialPort(this);
    m_client = new Client(this);

    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(m_client, SIGNAL(handshakeResponse(bool)), this, SLOT(handleHandshakeResponse(bool)));
    connect(m_client,SIGNAL(infoStatusResponse(bool, status_hdr_t*,QList<fileheader_data_t>*)),SLOT(handleInfoStatusResponse(bool , status_hdr_t*,QList<fileheader_data_t>*)));

    connect(m_client, SIGNAL(sendFileHeaderResponse(bool)), this, SLOT(handleSendFileHeaderResponse(bool)));
    connect(m_client, SIGNAL(sendFileChunkResponse(bool,uint32_t, uint32_t)), this, SLOT(handleSendFileChunkResponse(bool,uint32_t, uint32_t)));

    connect(m_client, SIGNAL(sendCommandResponse(bool)), this, SLOT(handleSendCommandResponse(bool)));
    connect(m_client,SIGNAL(bufferStatusChanged(buffer_status_t)),SLOT(handleStatusChanged(buffer_status_t)));
    connect(m_client,SIGNAL(bufferError(buffer_status_t)),SLOT(handleBufferError(buffer_status_t)));


    ui->progressBar->setValue(0);

    refreshSerialPortList();
    loadSampleRateList();

}

MainWindow::~MainWindow()
{
  delete ui;
  delete m_serialPort;
  delete m_client;
}


void MainWindow::on_toolButton_Previous_clicked()
{
  log(QString("Enviando Comando de Reproduccion 'Previous' ..."));
  m_client->sendPlaybackCommandRequest(PLAYBACK_COMMAND_PREVIOUS);
}

void MainWindow::on_toolButton_Pause_clicked()
{
  log(QString("Enviando Comando de Reproduccion 'Pause' ..."));
  m_client->sendPlaybackCommandRequest(PLAYBACK_COMMAND_PAUSE);
}

void MainWindow::on_toolButton_Play_clicked()
{
  log(QString("Enviando Comando de Reproduccion 'Play' ..."));
  m_client->sendPlaybackCommandRequest(PLAYBACK_COMMAND_PLAY);
}

void MainWindow::on_toolButton_Next_clicked()
{
  log(QString("Enviando Comando de Reproduccion 'Next' ..."));
  m_client->sendPlaybackCommandRequest(PLAYBACK_COMMAND_NEXT);
}

void MainWindow::on_toolButton_Upload_clicked()
{

  QString program = "ffmpeg";
  QStringList arguments;
  QProcess ffmpeg;
  QString filename;
  QString shortFilename;
  int sampleRate;

  filename = QFileDialog::getOpenFileName( this,"Seleccionar Archivo de Audio", "", "Archivo de Audio (*.wav *.mp3)");
  QFileInfo fileInfo(filename);
  shortFilename = fileInfo.fileName().toUpper();

  sampleRate = ui->comboBox_SampleRate->currentData().toInt();
  m_tmpFile = new QTemporaryFile(this);


  if (m_tmpFile->open()) {
    // this is only to get a valid tmp filename, so i close it
    //m_tmpFile->close();

    // contruye el comando: ffmpeg -i source -ac 1 -sample_fmt u8 -acodec pcm_u8 -f u8 -y -ar 8000 /tmp.file

    arguments << "-i" << filename;
    arguments << "-ac" << "1"; // audo channels: mono
    arguments << "-sample_fmt" << "u8"; //8 bit sample depth
    arguments << "-acodec" << "pcm_u8"; // audio codec: pcm 8 bit
    arguments << "-f" << "u8"; // format is PCM... headless WAV
    arguments << "-y"; //overwrite if file exists... it will exists
    arguments << "-ar" <<  QString::number(sampleRate); // audio sample rate
    arguments << m_tmpFile->fileName();

    log(QString("Ejecutando: %1 %2").arg(program).arg(arguments.join(" ")));
    ffmpeg.start(program, arguments);

    log(QString("waitForStarted ..."));
    ffmpeg.waitForStarted();
    log(QString("Started."));
    log(QString("waitForFinished ..."));

    ffmpeg.waitForFinished();
    log(QString("Finished."));

    log(QString("StandardError:"));
    log(QString("=================="));
    log(ffmpeg.readAllStandardError());
    log(QString("StandardOutput:"));
    log(QString("=================="));
    log(ffmpeg.readAllStandardOutput());
    log(QString("=================="));

    log(QString("Enviando audio..."));
    m_client->sendFile(m_tmpFile, sampleRate, shortFilename);

  }



}

void MainWindow::on_pushButton_RefreshPortList_clicked()
{
  log(QString("Actualizando lista de puertos serie."));
  refreshSerialPortList();
}

void MainWindow::on_pushButton_Connect_clicked()
{
  if (m_serialPort->isOpen())
    closeSerialPort();
  else
    openSerialPort();

  updateConnectButtonLabel();


  if (m_serialPort->isOpen()){
    m_client->setSerialPort(m_serialPort);
    m_client->sendHandshakeRequest();
    log(QString("Iniciando handshake..."));

  }


}




void MainWindow::refreshSerialPortList()
{
  ui->comboBox_PortList->clear();
  foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
    ui->comboBox_PortList->addItem(info.portName(), info.portName());
  }
}

void MainWindow::loadSampleRateList()
{
  ui->comboBox_SampleRate->clear();
  ui->comboBox_SampleRate->addItem("8 Khz", 8000);
  ui->comboBox_SampleRate->addItem("11 Khz", 11025);
  ui->comboBox_SampleRate->addItem("22 Khz", 22050);
  ui->comboBox_SampleRate->addItem("44 Khz", 44100);

}




void MainWindow::updateConnectButtonLabel()
{
  if(m_serialPort->isOpen())
    ui->pushButton_Connect->setText("Desconectar");
  else
    ui->pushButton_Connect->setText("Conectar");
}


void MainWindow::openSerialPort()
{
  log(QString("Intentando abrir puerto serie."));

  if (ui->comboBox_PortList->currentData().isNull()){
    QMessageBox::critical(this, "Error de Conecxion","seleccione un puerto válido");
    ui->statusBar->showMessage(tr("Seleccione un puerto valido"));
    log(QString("El puerto no es válido."));
    return;
  }

  m_serialPort->setPortName(ui->comboBox_PortList->currentData().toString());
  m_serialPort->setBaudRate(QSerialPort::Baud38400);
  m_serialPort->setDataBits(QSerialPort::Data8);
  m_serialPort->setParity(QSerialPort::NoParity);
  m_serialPort->setStopBits(QSerialPort::OneStop);
  m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

  if(m_serialPort->open(QIODevice::ReadWrite))
  {
    ui->statusBar->showMessage("Conectado a " + ui->comboBox_PortList->currentData().toString());
    log(QString("Conectado a %1 .").arg(ui->comboBox_PortList->currentText()));
  }
  else
  {
    ui->statusBar->showMessage(tr("Error al abrir puerto"));
    log(QString("Error al abrir puerto %1 .").arg(ui->comboBox_PortList->currentText()));
  }


}

void MainWindow::closeSerialPort()
{
  log(QString("Cerrando puerto serie."));
  m_serialPort->close();
  ui->listWidget_DeviceAudios->clear();
  ui->groupBox_DeviceControl->setEnabled(false);
  ui->statusBar->showMessage(tr("No Conectado"));
}

void MainWindow::log(QString msg)
{
  msg.append("\n");
  ui->plainTextEdit_Log->insertPlainText(msg);
  ui->plainTextEdit_Log->centerCursor();
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
  if (error == QSerialPort::ResourceError) {
    log(QString("Error Critico %1.").arg(m_serialPort->errorString()));
    QMessageBox::critical(this, "Error Critico", m_serialPort->errorString());
    closeSerialPort();
  }
}



void MainWindow::handleHandshakeResponse(bool success)
{
  if(success)
  {
    log(QString("Handshake aceptado."));
    m_client->getDeviceStatus();
    log(QString("Solicitando estado del dispositivo..."));
  }
  else
  {
    log(QString("Dispositivo no detectado."));
  }


}

void MainWindow::handleInfoStatusResponse(bool success, status_hdr_t* status, QList<fileheader_data_t> * fileList)
{
  ui->listWidget_DeviceAudios->clear();
  ui->groupBox_DeviceControl->setEnabled(success);

  if(success)
  {
    log(QString("Estado del dispositivo recibida."));
    log(QString(" --> SD conectada: %1.").arg(status->sd_connected));
    log(QString(" --> Espacio disponible: %1.").arg(status->available_space));
    log(QString(" --> Espacio Total: %1.").arg(status->total_space));
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
    log(QString("Comando de Reproduccion Aceptado."));
  }
  else
  {
    log(QString("Comando de Reproduccion Rechazado."));
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

    float progress = float(chunk_id) / float(chunksCount);
    ui->progressBar->setValue((int) qRound(progress*100.0f));

    if(chunk_id==chunksCount-1)
    {
      //todo: send a confirmation request...
      log(QString("Ultimo chunk de archivo recibido."));
      m_client->getDeviceStatus();
      log(QString("Solicitando estado del dispositivo..."));

      //ui->groupBox_DeviceControl->setEnabled(true);
      //ui->groupBox_AudioProgress->setEnabled(false);
    }

  }
  else
  {
    log(QString("Fallo la recepción de chunk %1 .").arg(chunk_id));
    ui->groupBox_DeviceControl->setEnabled(true);
    ui->groupBox_AudioProgress->setEnabled(false);

  }
}

void MainWindow::handleBufferError(buffer_status_t bufferStatus)
{
  Q_UNUSED(bufferStatus);
  //log(QString("      * serial buffer error code: %1 * ").arg(bufferStatus));
}

void MainWindow::handleStatusChanged(buffer_status_t bufferStatus)
{
  Q_UNUSED(bufferStatus);
  //log(QString("      * serial buffer status changed: %1 * ").arg(bufferStatus));

}




