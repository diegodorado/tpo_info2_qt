#include "client.h"

Client::Client(QObject *parent) :
  QObject(parent)
{
  m_audioFile = NULL;
  m_pendingMessagesMask.resize(MAX_CONCURRENT_MESSAGES);
  m_pendingMessagesMask.fill(false);
  m_serialPort = new QSerialPort(this);
  m_deviceStatus = new status_hdr_t;
  m_fileList = new QList<fileheader_data_t>();

  // a single signal!!!
  connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readSerialData()));

  //timers
  m_fileSendTimer = new QTimer(this);
  m_keepAliveTimer = new QTimer(this);
  m_deadLineTimer =  new QTimer(this);
  m_fileSendTimer->setInterval(150); //fake some delay
  m_keepAliveTimer->setInterval(1500);
  m_deadLineTimer->setInterval(5000);
  connect(m_fileSendTimer, SIGNAL(timeout()), this, SLOT(processFileSend()));
  connect(m_keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
  connect(m_deadLineTimer, SIGNAL(timeout()), this, SLOT(deadLine()));
  m_keepAliveTimer->start();


}

Client::~Client()
{
  delete m_serialPort;
  delete m_fileList;
  delete m_deviceStatus;

  delete m_fileSendTimer;
  delete m_keepAliveTimer;
  delete m_deadLineTimer;

}

void Client::sendHandshakeRequest()
{
  message_hdr_t request;
  if (!pendingFull())
  {
    request.data_length = 0;
    request.is_response = 0;
    request.msg_type = MESSAGE_HANDSHAKE;
    //no data.... bodyless message
    sendMessageRequest(&request, NULL);
  }
}

void Client::sendCommandRequest(command_type_t command)
{
  message_hdr_t request;
  if (canSendMessage())
  {
    request.data_length = 1;
    request.is_response = 0;
    request.msg_type = MESSAGE_COMMAND;
    sendMessageRequest(&request, (uint8_t*) &command);
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
  }

}

QSerialPort *Client::getSerialPort()
{
  return m_serialPort;
}

bool Client::openSerialPort(QString port, uint16_t baudRate)
{
  m_serialPort->setPortName(port);
  m_serialPort->setBaudRate(baudRate);
  m_serialPort->setDataBits(QSerialPort::Data8);
  m_serialPort->setParity(QSerialPort::NoParity);
  m_serialPort->setStopBits(QSerialPort::OneStop);
  m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
  m_deviceConnected = -1;
  return m_serialPort->open(QIODevice::ReadWrite);
}

void Client::closeSerialPort()
{
  updateDeviceStatus(false);
  if(m_serialPort->isOpen())
    m_serialPort->close();
}

void Client::sendFile(QFile *file, uint32_t sampleRate, QString filename)
{
  m_audioFile = file;

  m_fileHeader.sample_rate = sampleRate;
  m_fileHeader.length =  m_audioFile->size();
  strncpy(m_fileHeader.filename, filename.toLatin1().data() ,8);

  m_fileHeader.chunks_count = m_fileHeader.length / FILECHUNK_SIZE;
  if((m_fileHeader.length % FILECHUNK_SIZE) > 0)
    m_fileHeader.chunks_count++;

  m_chunkIndex = 0;
  m_fileHeaderSent = false;
  m_fileHeaderAcepted = false;
  m_fileSendTimer->start();
}

bool Client::canSendMessage()
{
  //check if connected and not pendingFull
  return (m_deviceConnected==1) && !pendingFull();
}

bool Client::pendingFull(){
  //check if there is an available msg id
  return (m_pendingMessagesMask.count(false) == 0);
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
  int msg_id = -1;
  // assigns a message id and flags it to check response later
  for(int i=0;i < MAX_CONCURRENT_MESSAGES; i++)
    if( !m_pendingMessagesMask.testBit(i))
    {
      msg_id = i;
      break;
    }

  if(msg_id==-1)
    return;
  else
  {
    message->msg_id = msg_id;
    m_pendingMessagesMask.setBit(msg_id);

    m_keepAliveTimer->start(); // restart
    if(!m_deadLineTimer->isActive())
      m_deadLineTimer->start();

    sendMessage(message, data);
    return;

  }


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
        readMessageFromBuffer();
        break;
      case BUFFER_ERROR_SOF_EXPECTED:
      case BUFFER_ERROR_CHECKSUM:
      case BUFFER_ERROR_EOF_EXPECTED:
      case BUFFER_ERROR_INVALID_MSG_LENGTH:
        emit log(QString("Message Buffer Error: %1").arg(m_bufferStatus));
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
    emit log(QString("Message Error: NULL_MESSAGE ."));
    free(message);
    return;
  }

  if(message->msg_type >= MESSAGE_MAX_VALID_TYPE){
    emit log(QString("Message Error: INVALID_MESSAGE_TYPE ."));
    free(message);
    return;
  }

  emit log(QString("Message: %1   type: %2 id:%3.").arg(message->is_response).arg(message->msg_type).arg(message->msg_id));


  if(message->is_response)
    //check if a request was made
    if(m_pendingMessagesMask.testBit(message->msg_id))
    {
      m_pendingMessagesMask.clearBit(message->msg_id);
      processMessageResponse(message);
      updateDeviceStatus(true);
    }
    else
    {
      emit log(QString("MessageError: RESPONSE_NOT_EXPECTED ."));
    }
  else{
    processMessageRequest(message);
  }

  // dont forget to free(msg)!
  free(message);

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
    emit log(QString("Send chunk: %1 .").arg(m_chunkIndex));
    sendMessageRequest(&request, (uint8_t*) ba.data());


    if(++m_chunkIndex>=m_fileHeader.chunks_count )
      finishOrCancelFileTransfer();

  }




}

void Client::processMessageResponse(message_hdr_t* message)
{

  switch(message->msg_type){
    case MESSAGE_HANDSHAKE:
      break;
    case MESSAGE_INFO_STATUS:
      processInfoStatusResponse(message);
      break;
    case MESSAGE_COMMAND:
      emit sendCommandResponse( * messageData(message) == STATUS_OK );
      break;
    case MESSAGE_FILEHEADER:
      if(* messageData(message) == STATUS_OK )
      {
        m_fileHeaderAcepted = true;
        emit sendFileHeaderResponse(true);
      }
      else
      {
        finishOrCancelFileTransfer();
        emit sendFileHeaderResponse(false );
      }


      break;

    case MESSAGE_FILECHUNK:
      processSendFileChunkResponse(message);
      break;
  }



}

void Client::processInfoStatusResponse(message_hdr_t* response)
{

  // first check: data_length must be at least sizeof(status_data_t)
  if(response->data_length < sizeof(status_hdr_t)){
    emit log("Message too short.");
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }

  for(uint8_t i = 0; i < sizeof(status_hdr_t) ; i++)
    *( (uint8_t*) m_deviceStatus + i)  = * ( messageData(response) + i );


  if(m_deviceStatus->files_count>32){
    emit log("Too many files detected.");
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }


  if(response->data_length != sizeof(status_hdr_t) + m_deviceStatus->files_count * sizeof(fileheader_data_t) ){
    emit log("Data length mismatch.");
    emit infoStatusResponse(false,NULL,NULL);
    return;
  }

  for(int i = 0; i<m_deviceStatus->files_count;i++)
  {
    fileheader_data_t fd;
     for(uint8_t j = 0; j < sizeof(fileheader_data_t) ; j++)
      *( (uint8_t*) &fd  + j)  = * ( messageData(response) + sizeof(status_hdr_t) + sizeof(fileheader_data_t) * i + j );

    m_fileList->append(fd);

  }

  emit infoStatusResponse(true, m_deviceStatus, m_fileList);

}

void Client::processSendFileChunkResponse(message_hdr_t* response)
{
  //todo: create a list for a send/recieved match
  filechunk_hdr_t data;
  data = *(filechunk_hdr_t*) messageData(response);
  emit sendFileChunkResponse((data.status ==0),data.chunk_id, m_fileHeader.chunks_count);

}

void Client::keepAlive()
{

  if (!m_serialPort->isOpen())
    return;

  sendHandshakeRequest();

}


void Client::deadLine()
{
  updateDeviceStatus(false);
}


void Client::updateDeviceStatus(bool connected)
{
  //emit log(QString("updateDeviceStatus: %1 %2 %3").arg(m_deviceConnected).arg(connected).arg(m_deviceConnected == (int) connected));

  if(connected)
    m_deadLineTimer->start(); // device responded, so restart timer. This must be called every time.


  // m_deviceConnected isnt bool because I need tristate: true, false, not_checked_yet
  // this is to trigger the signal only once, and for the first time also
  if(m_deviceConnected == (int) connected)
    return;

  m_deviceConnected = (int) connected;

  emit log(QString("Clear Serial Port: %1 ").arg(m_serialPort->clear()));



  if(!connected){
    m_deadLineTimer->stop();
    m_pendingMessagesMask.fill(false);
    finishOrCancelFileTransfer();
    messagesBufferClear();
  }

  emit deviceStatusChanged(connected);

}

void Client::finishOrCancelFileTransfer()
{
  m_fileSendTimer->stop();
  if(m_audioFile != NULL && m_audioFile->exists()){
    m_audioFile->remove();
    m_audioFile = NULL;
  }

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

void Client::processMessageRequest(message_hdr_t *message)
{
  switch(message->msg_type){
    case MESSAGE_HANDSHAKE:
      message->is_response = 1;
      sendMessageResponse(message, NULL);
      break;
    case MESSAGE_INFO_STATUS:
      sendFakeDeviceStatus(message);
      break;
    case MESSAGE_COMMAND:
      sendStatusResponse(message,STATUS_OK);
      break;
    case MESSAGE_FILEHEADER:
      sendStatusResponse(message,STATUS_OK);
      break;
    case MESSAGE_FILECHUNK:
      sendFakeChunkResponse(message);
      break;
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
  status.files_count = 8;

  QByteArray ba;



  for(uint8_t i = 0; i < sizeof(status_hdr_t) ; i++)
    ba.append( *( ( (uint8_t*) &status ) + i) );


  for(int i = 0; i<status.files_count;i++)
  {
    fileheader_data_t fd;
    fd.length = 0;
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




