#include "client.h"

Client::Client(QObject *parent) :
  QObject(parent)
{
  m_pendingMessagesMask.resize(0xFF);
  m_pendingMessagesMask.fill(false);
  m_messagesQueue = new QList<message_hdr_t*>();
  m_serialPort = NULL;

  m_fileList = new QList<fileheader_data_t>();


  m_queueTimer = new QTimer(this);
  m_fileSendTimer = new QTimer(this);

  m_handshakeResponseTimer = new QTimer(this);
  m_infoStatusResponseTimer = new QTimer(this);
  m_sendCommandResponseTimer = new QTimer(this);
  m_sendFileResponseTimer = new QTimer(this);

  connect(m_queueTimer, SIGNAL(timeout()), this, SLOT(processMessagesQueue()));
  connect(m_fileSendTimer, SIGNAL(timeout()), this, SLOT(processFileSend()));

  connect(m_handshakeResponseTimer, SIGNAL(timeout()), this, SLOT(handshakeResponseTimeout()));
  connect(m_infoStatusResponseTimer, SIGNAL(timeout()), this, SLOT(infoStatusResponseTimeout()));
  connect(m_sendCommandResponseTimer, SIGNAL(timeout()), this, SLOT(sendCommandResponseTimeout()));
  connect(m_sendFileResponseTimer, SIGNAL(timeout()), this, SLOT(sendFileResponseTimeout()));

  connect(this, SIGNAL(bufferReadyRead()), this, SLOT(readMessageFromBuffer()));
  connect(this, SIGNAL(bufferStatusChanged(buffer_status_t)), this, SLOT(handleBufferStatusChanged(buffer_status_t)));
  connect(this, SIGNAL(messageError(message_error_t)), this, SLOT(handleMessageError(message_error_t)));


}

Client::~Client()
{
  delete m_messagesQueue;
  delete m_fileList;
  delete m_queueTimer;
  delete m_fileSendTimer;
}


void Client::sendHandshakeRequest()
{
  message_hdr_t request;

  request.data_length = 0;
  request.is_response = 0;
  request.msg_type = MESSAGE_HANDSHAKE;

  if ( m_handshakeResponseTimer->isActive())
  {
    qDebug() << "Already waiting for handshake response.";
    emit handshakeResponse(false);
    return;
  }

  if (trySetMessageId(&request))
  {
    //no data.... bodyless message
    sendMessageRequest(&request, NULL);
    m_handshakeResponseTimer->start(RESPONSE_TIMEOUT_MS);
  }

}

void Client::sendPlaybackCommandRequest(playback_command_type_t command)
{
  message_hdr_t request;

  request.data_length = 1;
  request.is_response = 0;
  request.msg_type = MESSAGE_PLAYBACK_COMMAND;

  if ( m_sendCommandResponseTimer->isActive())
  {
    qDebug() << "Already waiting for playback command response.";
    emit sendCommandResponse(false);
    return;
  }

  if (trySetMessageId(&request))
  {
    //no data.... bodyless message
    sendMessageRequest(&request, (uint8_t*) &command);
    m_sendCommandResponseTimer->start(RESPONSE_TIMEOUT_MS);
  }
}

void Client::getDeviceStatus()
{
  m_fileList->clear();

  message_hdr_t request;

  request.data_length = 0;
  request.is_response = 0;
  request.msg_type = MESSAGE_INFO_STATUS;

  if ( m_infoStatusResponseTimer->isActive())
  {
    qDebug() << "Already waiting for MESSAGE_INFO_STATUS response.";
    //emit infoStatusResponse(false);
    return;
  }

  if (trySetMessageId(&request))
  {
    //no data.... bodyless message
    sendMessageRequest(&request, NULL);
    m_infoStatusResponseTimer->start(RESPONSE_TIMEOUT_MS);
  }

}



void Client::setSerialPort(QSerialPort *serialPort)
{
  m_queueTimer->stop();
  if(m_serialPort !=NULL){
    disconnect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
    disconnect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readSerialData()));
    disconnect(m_serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));
  }
  m_serialPort = serialPort;
  connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
  connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readSerialData()));
  connect(m_serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));
  m_queueTimer->start(); //fake some delay

}

void Client::sendFile(QString filename)
{
  if(m_audioFile.isOpen()){
    qDebug() << "a File was open... ";
    m_audioFile.close();
  }


  m_audioFile.setFileName(filename);
  m_audioFile.open(QIODevice::ReadOnly);

  m_fileHeaderSent = false;
  m_fileSendTimer->start();
}

bool Client::trySetMessageId(message_hdr_t *message)
{
  if (m_pendingMessagesMask.count(false) == 0)
    //message queue is full... wait for next iteration
    return false;

  message->msg_id = 0;
  while(m_pendingMessagesMask.at(message->msg_id) && message->msg_id<m_pendingMessagesMask.size())
    message->msg_id++;

  return true;

}

void Client::sendMessage(message_hdr_t* message, uint8_t* data)
{
  QByteArray d;

  uint8_t checksum = messageGetChecksum(message, data);

  d.append(START_OF_FRAME);
  d.append(message->data_length);
  d.append(message->msg_id);
  d.append(message->msg_full_type);

  for(int i = 0; i < message->data_length ; i++){
    d.append(*(data + i));
  }

  d.append(checksum);
  d.append(END_OF_FRAME);


  m_serialPort->write(d);
  qDebug() << "Mensaje enviado; id: " << message->msg_id << "type: " << message->msg_type << "resp: " << message->is_response << "length: " << message->data_length;
}

void Client::sendMessageRequest(message_hdr_t* message, uint8_t* data)
{
  if(m_pendingMessagesMask.testBit(message->msg_id))
  {
    emit messageError(message_error_t::MSG_ID_IN_USE);
  }
  else
  {
    m_pendingMessagesMask.setBit(message->msg_id);
    sendMessage(message, data);
    //todo: implement a timeout for the response
  }
}

void Client::sendMessageResponse(message_hdr_t* message, uint8_t* data)
{
  sendMessage(message, data);
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
        emit bufferError(m_bufferStatus);
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
  message_hdr_t* message = (message_hdr_t*) raw_data;


  for(int i=0;i<message->data_length;i++)
    qDebug() << *(raw_data+i);

  //data is a pointer, so casting raw data is only valid
  //for fixed size struct members
  //use wisely: message->length - 3 will tell size of data

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



  qDebug() << "Mensaje recibido; id: " << message->msg_id << "type: " << message->msg_type << "resp: " << message->is_response << "length: " << message->data_length;



  // dont forget to free(msg)!
  if(message->is_response)
    handleMessageResponse(message);
  else
    handleMessageRequest(message);


}

void Client::handleMessageRequest(message_hdr_t *message)
{
  // enqueu message
  m_messagesQueue->append(message);
}

void Client::handleMessageResponse(message_hdr_t *message)
{
  //check if a request was made
  if(m_pendingMessagesMask.testBit(message->msg_id))
  {
    m_pendingMessagesMask.clearBit(message->msg_id);
    processMessageResponse(message);
  }
  else
  {
    emit messageError(message_error_t::RESPONSE_NOT_EXPECTED);
  }
  free(message);

}

void Client::handleSerialError(QSerialPort::SerialPortError error)
{
  qDebug() << "Critical Error" << error;
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


void Client::processFileSend()
{
  const int wavHeaderSize = 44;
  const int chunkSize = 200;

  message_hdr_t request;

  if (trySetMessageId(&request))
    //message queue is full... wait for next iteration
    return;


  if(!m_fileHeaderSent)
  {
    fileheader_data_t fileheader_data;

    QFileInfo fileInfo(m_audioFile.fileName());

    m_fileToSendSize = m_audioFile.size() - wavHeaderSize;
    m_fileToSendChunksCount = m_fileToSendSize / chunkSize;
    if((m_fileToSendSize % chunkSize) > 0)
      m_fileToSendChunksCount ++;
    m_fileToSendChunkIndex = 0;


    fileheader_data.filesize = m_fileToSendSize;
    fileheader_data.chunks_count = m_fileToSendChunksCount;
    strncpy(fileheader_data.filename,fileInfo.fileName().toUpper().toLatin1().data(),8);

    request.data_length = sizeof(fileheader_data);
    request.is_response = 0;
    request.msg_type = MESSAGE_FILEHEADER;
    //cast data struct as uint8_t pointer
    sendMessageRequest(&request, (uint8_t*) &fileheader_data);
    m_fileHeaderSent = true;

  }
  else
  {
    m_audioFile.seek(wavHeaderSize + chunkSize*m_fileToSendChunkIndex);
    QByteArray ba = m_audioFile.read(chunkSize);
    //QByteArray chunkIndex((char*)m_fileToSendChunkIndex,sizeof(m_fileToSendChunkIndex));
    //todo: prepend chunk_id
    //data on filechunk message have a chunk_id prepended
    //ba.prepend(chunkIndex);
    request.data_length = ba.size();
    request.msg_type = MESSAGE_FILECHUNK;
    request.is_response = 0;
    sendMessageRequest(&request, (uint8_t*) ba.data());


    if(++m_fileToSendChunkIndex>=m_fileToSendChunksCount)
    {
      m_fileSendTimer->stop();
      //sendHandshakeRequest();
      //todo:send a confirmation request??
    }

  }




}



void Client::processMessageResponse(message_hdr_t* message)
{

  switch(message->msg_type){
    case MESSAGE_HANDSHAKE:
      m_handshakeResponseTimer->stop(); //prevent a timeout
      emit handshakeResponse( * messageData(message) == STATUS_OK );
      break;
    case MESSAGE_INFO_STATUS:
      processInfoStatusResponse(message);
      break;
    case MESSAGE_PLAYBACK_COMMAND:
      m_sendCommandResponseTimer->stop(); //prevent a timeout
      emit sendCommandResponse( * messageData(message) == STATUS_OK );
      break;
    case MESSAGE_FILEHEADER:
      break;
    case MESSAGE_FILECHUNK:
      break;
  }



}


void Client::processInfoStatusResponse(message_hdr_t* response)
{

  m_infoStatusResponseTimer->stop();

  status_hdr_t status;

  // first check: response.length - 3 must be at least sizeof(status_data_t)
  if(response->data_length < sizeof(status_hdr_t)){
    qDebug() << "Message too short.";
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }

  for(uint8_t i = 0; i < sizeof(status_hdr_t) ; i++)
    *( (uint8_t*) &status + i)  = * ( messageData(response) + i );


  if(status.sd_connected>1){
    qDebug() << "Invalid sd_connected value.";
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }

  if(status.files_count>32){
    qDebug() << "Too many files detected.";
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }


  if(response->data_length != sizeof(status_hdr_t) + status.files_count * sizeof(fileheader_data_t) ){
    qDebug() << "Data length mismatch.";
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }

  for(int i = 0; i<status.files_count;i++)
  {
    fileheader_data_t fd;
     for(uint8_t j = 0; j < sizeof(fileheader_data_t) ; j++)
      *( (uint8_t*) &fd  + j)  = * ( messageData(response) + sizeof(status_hdr_t) + sizeof(fileheader_data_t) * i + j );

    m_fileList->append(fd);

  }


  m_deviceStatus = status;
  emit infoStatusResponse(true, &m_deviceStatus, m_fileList);

}



void Client::handshakeResponseTimeout()
{
  m_handshakeResponseTimer->stop();
  emit handshakeResponse(false);
}

void Client::infoStatusResponseTimeout()
{
  m_infoStatusResponseTimer->stop();
  emit infoStatusResponse(false,NULL,NULL);
}

void Client::sendCommandResponseTimeout()
{
  m_sendCommandResponseTimer->stop();
  emit sendCommandResponse(false);
}

void Client::sendFileResponseTimeout()
{
  m_sendFileResponseTimer->stop();
  emit sendFileResponse(false);
}


/*
 * WARNING:
 * This data is all fake
 * It is here for testing purposes only.
 * Normally device wont send any request but only responses
 *
*/
void Client::processMessagesQueue()
{

  //fixme: hardcoded max of message to handle
  for (int i = 0; i < m_messagesQueue->size() &&  i < 2; i++) {

    message_hdr_t* message = m_messagesQueue->at(i);


    switch(message->msg_type){
      case MESSAGE_HANDSHAKE:
        sendStatusResponse(message,STATUS_OK);
        //sendStatusResponse(message,STATUS_ERROR);
        break;
      case MESSAGE_INFO_STATUS:
        sendFakeDeviceStatus(message);
        break;
      case MESSAGE_PLAYBACK_COMMAND:
        sendStatusResponse(message,STATUS_OK);
        break;
      case MESSAGE_FILEHEADER:
        sendStatusResponse(message,STATUS_OK);
        qDebug() << "MESSAGE_FILEHEADER: " << "length: " << message->data_length;
        break;
      case MESSAGE_FILECHUNK:
        sendStatusResponse(message,STATUS_OK);
        qDebug() << "MESSAGE_FILECHUNK: " << "length: " << message->data_length ;
        break;
    }


    free(message);
    m_messagesQueue->removeAt(i);


  }

}




void Client::sendStatusResponse(message_hdr_t* request, status_id_t status)
{
  message_hdr_t response;

  response.data_length = 1;
  response.msg_id = request->msg_id;
  response.msg_type = request->msg_type;
  response.is_response = 1;
  sendMessageResponse(&response, (uint8_t*)&status);

}

void Client::sendFakeDeviceStatus(message_hdr_t *request)
{
  message_hdr_t response;
  status_hdr_t status;
  status.sd_connected = 1;
  status.total_space = 150;
  status.available_space = 75;
  status.files_count = 8;

  QByteArray ba;



  for(uint8_t i = 0; i < sizeof(status_hdr_t) ; i++)
    ba.append( *( ( (uint8_t*) &status ) + i) );


  for(int i = 0; i<status.files_count;i++)
  {
    fileheader_data_t fd;
    fd.filesize = m_fileToSendSize;
    fd.chunks_count = m_fileToSendChunksCount;
    strncpy(fd.filename,QString("AUDIO_%1").arg(i).toLatin1().data(),8);

    for(uint8_t j = 0; j < sizeof(fileheader_data_t) ; j++)
      ba.append( *( ( (uint8_t*) &fd ) + j) );

  }


  response.msg_id = request->msg_id;
  response.msg_type = request->msg_type;
  response.is_response = 1;
  response.data_length = ba.size();
  sendMessageResponse(&response, (uint8_t*) ba.data());



}




