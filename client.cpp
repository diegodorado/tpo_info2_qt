#include "client.h"

Client::Client(QObject *parent) :
  QObject(parent)
{
  m_pendingMessagesMask.resize(0xFF);
  m_pendingMessagesMask.fill(false);
  m_messagesQueue = new QList<message_t*>();
  m_serialPort = new QSerialPort(this);


  m_queueTimer = new QTimer(this);


  connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
  connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readSerialData()));
  connect(m_serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));
  connect(this, SIGNAL(bufferReadyRead()), this, SLOT(readMessageFromBuffer()));
  connect(this, SIGNAL(bufferStatusChanged(buffer_status_t)), this, SLOT(handleBufferStatusChanged(buffer_status_t)));
  connect(this, SIGNAL(messageError(message_error_t)), this, SLOT(handleMessageError(message_error_t)));

  openSerialPort();

  connect(m_queueTimer, SIGNAL(timeout()), this, SLOT(processMessagesQueue()));
  m_queueTimer->start(1000); //fake some delay

  sendFileChunk();

}


void Client::sendHandshakeRequest()
{
  message_t request;

  request.length = 3;
  request.is_response = 0;
  request.msg_type = MESSAGE_HANDSHAKE;
  request.msg_id = 0;
  //no data.... bodyless message
  sendMessageRequest(&request);

}


void Client::sendStatusResponse(message_t* request, status_id_t status)
{
  message_t response;

  response.length = 4;
  response.msg_id = request->msg_id;
  response.msg_type = request->msg_type;
  response.is_response = 1;
  response.data = (uint8_t*)&status;
  //no data.... bodyless message
  sendMessageResponse(&response);

}



void Client::sendFileHeader(fileheader_data_t* fileheader_data)
{
  uint8_t  length = sizeof(*fileheader_data);
  uint8_t* data = (uint8_t*) fileheader_data;  //cast data struct as uint8_t pointer
  message_t message;

  strcpy(fileheader_data->filename,"qwerpoiu");
  fileheader_data->filesize = 1024;
  fileheader_data->chunks_count = 10;


  message.length = length + 3;
  message.is_response = 0;
  message.msg_type = MESSAGE_FILEHEADER;
  message.data = data;

  for(int i=0;i<=255;i++){
    message.msg_id = i;
    sendMessage(&message);

  }


}

void Client::sendTest()
{
  sendHandshakeRequest();
  sendFileChunk();
  //fileheader_data_t fileheader_data;
  //openSerialPort();
  //sendFileHeader(&fileheader_data);


}

void Client::sendFileChunk()
{

  QString filename = "/home/diego/music/8bit.wav";
  m_audioFile.setFileName(filename);
  m_audioFile.open(QIODevice::ReadOnly);

  QByteArray audioData = m_audioFile.read(200);


  message_t request;

  request.length = 3 + audioData.size();
  request.msg_id = 1;
  request.msg_type = MESSAGE_FILECHUNK;
  request.is_response = 0;
  request.data = (uint8_t*) audioData.data();
  sendMessageRequest(&request);


  m_audioFile.close();
}

void Client::sendMessage(message_t* message)
{
  QByteArray d;

  uint8_t checksum = messageGetChecksum(message);

  d.append(START_OF_FRAME);
  d.append(message->length);
  d.append(message->msg_id);
  d.append(message->msg_full_type);

  for(int i = 0; i < message->length - 3 ; i++){
    d.append(*(message->data+i));
  }

  d.append(checksum);
  d.append(END_OF_FRAME);

  m_serialPort->write(d);
}

void Client::sendMessageRequest(message_t *message)
{
  if(m_pendingMessagesMask.testBit(message->msg_id))
  {
    emit messageError(message_error_t::MSG_ID_IN_USE);
  }
  else
  {
    m_pendingMessagesMask.setBit(message->msg_id);
    sendMessage(message);
    //todo: implement a timeout for the response
  }
}

void Client::sendMessageResponse(message_t *message)
{
  sendMessage(message);
}

void Client::openSerialPort()
{
    m_serialPort->setPortName("/dev/ttyUSB0");
    m_serialPort->setBaudRate(QSerialPort::Baud9600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    if (m_serialPort->open(QIODevice::ReadWrite)) {
      qDebug() <<  "Puerto abierto";
    } else {
      qDebug() <<  "Error:  " << m_serialPort->errorString();
    }
}

void Client::closeSerialPort()
{
    m_serialPort->close();
}

void Client::readSerialData()
{
  QByteArray data = m_serialPort->readAll();
  buffer_status_t previous_status;

  //push received data to buffer
  for(int i = 0; i < data.size(); i++)
    messagesBufferPush( (uint8_t) data.at(i) );

  do{
    previous_status = m_bufferStatus;
    m_bufferStatus = messagesBufferProcess();
    if(previous_status !=m_bufferStatus)
      emit bufferStatusChanged(m_bufferStatus);

    switch(m_bufferStatus){
      case BUFFER_NOT_SOF:
      case BUFFER_SOF:
      case BUFFER_IN_MSG:
      case BUFFER_EOF:
        //do no take any special action here
        break;
      case BUFFER_MSG_OK:
        emit bufferReadyRead();
        break;
      case BUFFER_ERROR_SOF_EXPECTED:
      case BUFFER_ERROR_CHECKSUM:
      case BUFFER_ERROR_EOF_EXPECTED:
      case BUFFER_ERROR_INVALID_MSG_LENGTH:
        emit bufferError(previous_status);
        break;
    }
    // this while is controlled
    // it only happens if more than one messages
    // are received at once
  } while(m_bufferStatus==BUFFER_MSG_OK);

}

void Client::readMessageFromBuffer()
{
  uint8_t* raw_data = messagesBufferPop();
  message_t* message = (message_t*) raw_data;

  //data is a pointer, so casting raw data is only valid
  //for fixed size struct members
  //use wisely: message->length - 3 will tell size of data
  message->data = (raw_data+3);

  //perform some common validations

  if(message == NULL){
    emit messageError(message_error_t::NULL_MESSAGE);
    free(message);
    return;
  }

  if(message->msg_type >= MESSAGE_MAX_VALID_TYPE){
    emit messageError(message_error_t::INVALID_MESSAGE_TYPE);
    free(message);
    return;
  }



  qDebug() << "id: " << message->msg_id << "type: " << message->msg_type << "resp: " << message->is_response << "length: " << message->length;



  // dont forget to free(msg)!
  if(message->is_response)
    handleMessageResponse(message);
  else
    handleMessageRequest(message);


}

void Client::handleMessageRequest(message_t *message)
{
  // enqueu message
  m_messagesQueue->append(message);
}

void Client::handleMessageResponse(message_t *message)
{
  //check if a request was made
  if(m_pendingMessagesMask.testBit(message->msg_id))
  {
    m_pendingMessagesMask.clearBit(message->msg_id);
    qDebug() << "handleMessageResponse "<< "(message->data): " << *(message->data);
  }
  else
  {
    emit messageError(message_error_t::RESPONSE_NOT_EXPECTED);
  }
  free(message);

}

void Client::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        qDebug() << "Critical Error" << m_serialPort->errorString();
        closeSerialPort();
    }
}

void Client::handleBufferError(buffer_status_t error)
{
  switch(error){
    case BUFFER_ERROR_SOF_EXPECTED:
      qDebug() << "BUFFER_ERROR_SOF_EXPECTED";
      break;
    case BUFFER_ERROR_CHECKSUM:
      qDebug() << "BUFFER_ERROR_CHECKSUM";
      break;
    case BUFFER_ERROR_EOF_EXPECTED:
      qDebug() << "BUFFER_ERROR_EOF_EXPECTED";
      break;
    case BUFFER_ERROR_INVALID_MSG_LENGTH:
      qDebug() << "BUFFER_ERROR_INVALID_MSG_LENGTH";
      break;
    default:
      qDebug() << "UNKNOWN ERROR";
      break;
  }
}

void Client::handleMessageError(Client::message_error_t messageError)
{
  qDebug() << "MessageError:" << messageError;

}

void Client::handleBufferStatusChanged(buffer_status_t status)
{
  Q_UNUSED(status);
  //qDebug() << "BufferStatusChanged to: " << status;

}

void Client::onBytesWritten(qint64 bytes)
{
  Q_UNUSED(bytes);

  //qDebug() << "BytesWritten: " <<bytes;

}

void Client::processMessagesQueue()
{

  for (int i = 0; i < m_messagesQueue->size(); i++) {

    message_t* message = m_messagesQueue->at(i);


    switch(message->msg_type){
      case MESSAGE_HANDSHAKE:
        sendStatusResponse(message,STATUS_OK);
        //sendStatusResponse(message,STATUS_ERROR);
        break;
      case MESSAGE_PLAYBACK_COMMAND:
        break;
      case MESSAGE_FILEHEADER:
        break;
      case MESSAGE_FILECHUNK:
        sendStatusResponse(message,STATUS_OK);
        qDebug() << "MESSAGE_FILECHUNK: " << "length: " << message->length  << "first data: : " << *(message->data);
        break;
      case MESSAGE_INFO_AVAILABLE_SPACE:
        //do no take any special action here
        break;
      case MESSAGE_INFO_STATUS:
        //do no take any special action here
        break;
      case MESSAGE_INFO_FILELIST:
        //do no take any special action here
        break;
    }


    free(message);
    m_messagesQueue->removeAt(i);


  }

}
