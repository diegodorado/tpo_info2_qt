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

    m_serialPort = new QSerialPort(this);
    m_client = new Client(this);

    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(m_client, SIGNAL(handshakeResponse(bool)), this, SLOT(handleHandshakeResponse(bool)));
    connect(m_client,SIGNAL(infoStatusResponse(bool, status_hdr_t*,QList<fileheader_data_t>*)),SLOT(handleInfoStatusResponse(bool , status_hdr_t*,QList<fileheader_data_t>*)));
    connect(m_client, SIGNAL(sendCommandResponse(bool)), this, SLOT(handleSendCommandResponse(bool)));
    connect(m_client,SIGNAL(bufferStatusChanged(buffer_status_t)),SLOT(handleStatusChanged(buffer_status_t)));
    connect(m_client,SIGNAL(bufferError(buffer_status_t)),SLOT(handleBufferError(buffer_status_t)));




    ui->progressBar->setValue(0);

    refreshSerialPortList();

}

MainWindow::~MainWindow()
{
  delete ui;
  delete m_serialPort;
  delete m_client;
}
  /*
void MainWindow::onBytesWritten(qint64 bytes)
{
  Q_UNUSED(bytes);

  if(m_file.bytesAvailable()>1024){
    serial->write(m_file.read(1024));
  }
  float progress = 1.0f - float(m_file.bytesAvailable()) / float(m_file.size());
  ui->progressBar->setValue((int) qRound(progress*100.0f));


}

*/






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

  //QString fileName = QFileDialog::getOpenFileName( this,tr("Open Image"), "", tr("WAV Files (*.wav)"));
  //qDebug() << fileName;

  QString filename = "/home/diego/music/8bit.wav";
  //QByteArray data = m_file.read(sizeof(wav_hdr_t));
  //wav_hdr_t* wav_header = (wav_hdr_t*) data.data();
  //qDebug() << wav_header->ChunkID;


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
  m_serialPort->setBaudRate(QSerialPort::Baud9600);
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

  //qDebug() << msg;
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
        ui->listWidget_DeviceAudios->addItem(QString(file_header.filename));
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

void MainWindow::handleBufferError(buffer_status_t bufferStatus)
{
  //log(QString("      * serial buffer error code: %1 * ").arg(bufferStatus));
}

void MainWindow::handleStatusChanged(buffer_status_t bufferStatus)
{
  //log(QString("      * serial buffer status changed: %1 * ").arg(bufferStatus));

}



