#include "client.h"

Client::Client(QObject *parent) :
  QObject(parent)
{
  m_deviceConnected = false;
  m_pendingMessagesMask.resize(32); // no more than 32 concurrent unfinished messages
  m_pendingMessagesMask.fill(false);
  m_messagesQueue = new QList<message_hdr_t*>();
  m_serialPort = new QSerialPort(this);
  m_fileList = new QList<fileheader_data_t>();

  m_queueTimer = new QTimer(this);
  m_fileSendTimer = new QTimer(this);
  m_keepAliveTimer = new QTimer(this);
  m_infoStatusResponseTimer = new QTimer(this);
  m_sendCommandResponseTimer = new QTimer(this);
  m_sendFileResponseTimer = new QTimer(this);


  connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
  connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readSerialData()));

  connect(m_queueTimer, SIGNAL(timeout()), this, SLOT(processMessagesQueue()));
  connect(m_fileSendTimer, SIGNAL(timeout()), this, SLOT(processFileSend()));
  connect(m_keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
  connect(m_infoStatusResponseTimer, SIGNAL(timeout()), this, SLOT(infoStatusResponseTimeout()));
  connect(m_sendCommandResponseTimer, SIGNAL(timeout()), this, SLOT(sendCommandResponseTimeout()));
  connect(m_sendFileResponseTimer, SIGNAL(timeout()), this, SLOT(sendFileResponseTimeout()));

  connect(this, SIGNAL(bufferReadyRead()), this, SLOT(readMessageFromBuffer()));
  connect(this, SIGNAL(messageError(message_error_t)), this, SLOT(handleMessageError(message_error_t)));


  m_queueTimer->start(1); //fake some delay
  m_keepAliveTimer->start(KEEP_ALIVE_FREQUENCY);

}

Client::~Client()
{
  delete m_messagesQueue;
  delete m_serialPort;
  delete m_fileList;

  delete m_queueTimer;
  delete m_fileSendTimer;
  delete m_keepAliveTimer;
  delete m_infoStatusResponseTimer;
  delete m_sendCommandResponseTimer;
  delete m_sendFileResponseTimer;

}

void Client::sendHandshakeRequest()
{
  message_hdr_t request;

  if (canSendMessage())
  {
    request.data_length = 0;
    request.is_response = 0;
    request.msg_type = MESSAGE_HANDSHAKE;
    //no data.... bodyless message
    sendMessageRequest(&request, NULL);
  }

}

void Client::sendPlaybackCommandRequest(playback_command_type_t command)
{
  message_hdr_t request;


  if (canSendMessage())
  {
    request.data_length = 1;
    request.is_response = 0;
    request.msg_type = MESSAGE_PLAYBACK_COMMAND;
    sendMessageRequest(&request, (uint8_t*) &command);
    m_sendCommandResponseTimer->start(KEEP_ALIVE_FREQUENCY);
  }
}

void Client::getDeviceStatus()
{
  m_fileList->clear();

  message_hdr_t request;

  if (canSendMessage())
  {
    request.data_length = 0;
    request.is_response = 0;
    request.msg_type = MESSAGE_INFO_STATUS;
    //no data.... bodyless message
    sendMessageRequest(&request, NULL);
    m_infoStatusResponseTimer->start(KEEP_ALIVE_FREQUENCY);
  }

}

QSerialPort *Client::getSerialPort()
{
  return m_serialPort;
}

void Client::sendFile(QFile *file, uint32_t sampleRate, QString filename)
{
  m_audioFile = file;

  m_fileHeader.sample_rate = sampleRate;
  m_fileHeader.filesize =  m_audioFile->size();
  strncpy(m_fileHeader.filename, filename.toLatin1().data() ,8);

  m_fileHeader.chunks_count = m_fileHeader.filesize / FILECHUNK_SIZE;
  if((m_fileHeader.filesize % FILECHUNK_SIZE) > 0)
    m_fileHeader.chunks_count++;

  m_chunkIndex = 0;
  m_fileHeaderSent = false;
  m_fileHeaderAcepted = false;
  m_fileSendTimer->start(1); //fake some delay
}

bool Client::canSendMessage()
{
  //check if message queue is full
  return (m_pendingMessagesMask.count(false) > 0);
}

void Client::sendMessage(message_hdr_t* message, uint8_t* data)
{
  QByteArray d;
  uint16_t i;
  uint8_t checksum = messageGetChecksum(message, data);

  d.append(START_OF_FRAME);

  for(i = 0; i < sizeof(message_hdr_t); i++)
    d.append( *( ((uint8_t*) message) +i) );

  for(i = 0; i < message->data_length ; i++)
    d.append(*(data + i));

  d.append(checksum);
  d.append(END_OF_FRAME);
  m_serialPort->write(d);
}

void Client::sendMessageRequest(message_hdr_t* message, uint8_t* data)
{
  // assigns a message id and flags it to check response later
  message->msg_id = 0;
  while( m_pendingMessagesMask.at(message->msg_id) && message->msg_id < m_pendingMessagesMask.size() )
    message->msg_id++;

  m_pendingMessagesMask.setBit(message->msg_id);
  sendMessage(message, data);
}

void Client::sendMessageResponse(message_hdr_t* message, uint8_t* data)
{
  sendMessage(message, data);
}

void Client::readSerialData()
{
  QByteArray data = m_serialPort->readAll();

  //push received data to buffer
  for(int i = 0; i < data.size(); i++)
    messagesBufferPush( (uint8_t) data.at(i) );

  do{
    m_bufferStatus = messagesBufferProcess();

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
  if(error !=QSerialPort::NoError)
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

void Client::processFileSend()
{

  message_hdr_t request;

  if (!canSendMessage())
    //message queue is full... wait for next iteration
    return;


  if(!m_fileHeaderSent)
  {

    request.data_length = sizeof(m_fileHeader);
    request.is_response = 0;
    request.msg_type = MESSAGE_FILEHEADER;
    //cast data struct as uint8_t pointer
    sendMessageRequest(&request, (uint8_t*) &m_fileHeader);
    m_fileHeaderSent = true;

  }
  else if(m_fileHeaderAcepted)
  {

    m_audioFile->seek( FILECHUNK_SIZE * m_chunkIndex);

    char *buf = new char[FILECHUNK_SIZE];
    qint64 dataSize = m_audioFile->read(buf, FILECHUNK_SIZE);

    QByteArray ba;
    ba.append((char*) &m_chunkIndex,sizeof(m_chunkIndex));
    ba.append(buf, dataSize);
    delete buf;
    ba.resize(sizeof(m_chunkIndex) + dataSize);
    request.data_length = ba.size();
    request.msg_type = MESSAGE_FILECHUNK;
    request.is_response = 0;

    sendMessageRequest(&request, (uint8_t*) ba.data());


    if(++m_chunkIndex>=m_fileHeader.chunks_count )
    {
      m_fileSendTimer->stop();
      m_audioFile->remove();
      //todo: close file on error
    }

  }

  //2 seconds maximum time for any request to be responded
  //m_sendFileResponseTimer->start(2000);



}

void Client::processMessageResponse(message_hdr_t* message)
{

  switch(message->msg_type){
    case MESSAGE_HANDSHAKE:
      m_keepAliveResponded = true;
      // emit only once
      if(!m_deviceConnected)
      {
        emit deviceStatusChanged(true);
        m_deviceConnected = true;
      }

      break;
    case MESSAGE_INFO_STATUS:
      processInfoStatusResponse(message);
      break;
    case MESSAGE_PLAYBACK_COMMAND:
      m_sendCommandResponseTimer->stop(); //prevent a timeout
      emit sendCommandResponse( * messageData(message) == STATUS_OK );
      break;
    case MESSAGE_FILEHEADER:
      m_sendFileResponseTimer->stop();
      if(* messageData(message) == STATUS_OK )
      {
        m_fileHeaderAcepted = true;
        emit sendFileHeaderResponse(true);
      }
      else
      {
        m_fileSendTimer->stop();
        m_audioFile->remove();
        emit sendFileHeaderResponse(false );
      }


      break;

    case MESSAGE_FILECHUNK:
      m_keepAliveResponded = true; //fixme: multiple flichuncks blocks handshake response

      m_sendFileResponseTimer->stop();
      processSendFileChunkResponse(message);
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

void Client::processSendFileChunkResponse(message_hdr_t* response)
{
  filechunk_hdr_t data;
  data = *(filechunk_hdr_t*) messageData(response);
  emit sendFileChunkResponse((data.status ==0),data.chunk_id, m_fileHeader.chunks_count);

}

void Client::keepAlive()
{

  if (!m_serialPort->isOpen())
    return;

  // check if device was connected and didnt responded after KEEP_ALIVE_FREQUENCY
  if ( m_deviceConnected && !m_keepAliveResponded)
  {
    m_deviceConnected = false;
    // emit device disconnection
    emit deviceStatusChanged(false);

  }

  // send new handshake, not responded yet
  m_keepAliveResponded = false;
  sendHandshakeRequest();

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
  emit sendFileTimeout();
}







/*
 *
 *
 *
 *
 *
 *
 *
 * WARNING:
 * This data is all fake
 * It is here for testing purposes only.
 * Normally device wont send any request but only responses
 *
 *
 *
 *
 *
 *
*/
void Client::processMessagesQueue()
{

  for (int i = 0; i < m_messagesQueue->size(); i++) {

    message_hdr_t* message = m_messagesQueue->at(i);


    switch(message->msg_type){
      case MESSAGE_HANDSHAKE:
        sendStatusResponse(message,STATUS_OK);
        break;
      case MESSAGE_INFO_STATUS:
        sendFakeDeviceStatus(message);
        break;
      case MESSAGE_PLAYBACK_COMMAND:
        sendStatusResponse(message,STATUS_OK);
        break;
      case MESSAGE_FILEHEADER:
        sendStatusResponse(message,STATUS_OK);
        break;
      case MESSAGE_FILECHUNK:
        sendFakeChunkResponse(message);
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
    fd.filesize = 0;
    fd.chunks_count = 0;
    strncpy(fd.filename,QString("AUDIO_0%1").arg(i).toLatin1().data(),8);

    for(uint8_t j = 0; j < sizeof(fileheader_data_t) ; j++)
      ba.append( *( ( (uint8_t*) &fd ) + j) );

  }


  response.msg_id = request->msg_id;
  response.msg_type = request->msg_type;
  response.is_response = 1;
  response.data_length = ba.size();
  sendMessageResponse(&response, (uint8_t*) ba.data());



}

void Client::sendFakeChunkResponse(message_hdr_t *request)
{
  message_hdr_t response;
  filechunk_hdr_t data;
  data.status = 0;
  data.chunk_id = *(uint32_t*) messageData(request);

  QByteArray ba;

  for(uint8_t i = 0; i < sizeof(filechunk_hdr_t) ; i++)
    ba.append( *( ( (uint8_t*) &data ) + i) );


  response.msg_id = request->msg_id;
  response.msg_type = request->msg_type;
  response.is_response = 1;
  response.data_length = ba.size();
  sendMessageResponse(&response, (uint8_t*) ba.data());

}




