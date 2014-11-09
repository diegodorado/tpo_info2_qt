#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QByteArray>
#include <QBitArray>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include "protocol.h"


class Client : public QObject
{
  Q_OBJECT

public:
  explicit Client(QObject *parent = 0);
  ~Client();

  enum message_error_t {
    NULL_MESSAGE,
    INVALID_MESSAGE_TYPE,
    RESPONSE_NOT_EXPECTED,
    MSG_ID_IN_USE
  };

  void setSerialPort(QSerialPort* serialPort);

  void sendHandshakeRequest();

  void getDeviceStatus();

  void sendFile(QString filename);

private:
  QTimer* m_queueTimer;
  QTimer* m_fileSendTimer;
  QFile m_audioFile;
  QSerialPort* m_serialPort;
  QBitArray m_pendingMessagesMask;
  QList<message_t*>* m_messagesQueue;
  buffer_status_t m_bufferStatus;

  status_data_t m_deviceStatus;
  QList<fileheader_data_t>* m_fileList;


  bool m_fileHeaderSent;
  uint64_t  m_fileToSendSize;
  uint64_t  m_fileToSendChunksCount;
  uint64_t  m_fileToSendChunkIndex;

  bool trySetMessageId(message_t* message);

  void sendMessage(message_t* message);

  void sendMessageRequest(message_t* message);

  void sendMessageResponse(message_t* message);

  void handleMessageRequest(message_t* message);

  void handleMessageResponse(message_t* message);

  void processMessageResponse(message_t *message);

  void sendStatusResponse(message_t *request, status_id_t status);

  void sendFakeDeviceStatus(message_t *request);

  void processInfoStatusResponse(message_t *response);

private slots:
  void readSerialData();

  void handleSerialError(QSerialPort::SerialPortError bufferError);

  void onBytesWritten(qint64 bytes);

  void readMessageFromBuffer();

  void handleMessageError(message_error_t messageError);

  void handleBufferError(buffer_status_t bufferError);

  void handleBufferStatusChanged(buffer_status_t status);

  void processFileSend();

  void processMessagesQueue();


signals:
  void bufferError(buffer_status_t);

  void bufferReadyRead();

  void bufferStatusChanged(buffer_status_t);

  void messageError(message_error_t);

  void handshakeResponse(bool);

  void infoStatusResponse(status_data_t deviceStatus,  QList<fileheader_data_t>* fileList);

  void sendCommandResponse(bool);

  void sendFileResponse(bool);





public slots:

};

#endif // CLIENT_H

