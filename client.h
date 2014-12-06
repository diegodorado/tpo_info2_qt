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

  void sendPlaybackCommandRequest(playback_command_type_t command);

  void getDeviceStatus();

  void sendFile(QFile* file);

private:
  const int RESPONSE_TIMEOUT_MS = 5000;
  QTimer* m_queueTimer;
  QTimer* m_fileSendTimer;


  QTimer* m_handshakeResponseTimer;
  QTimer* m_infoStatusResponseTimer;
  QTimer* m_sendCommandResponseTimer;
  QTimer* m_sendFileResponseTimer;


  QFile* m_audioFile;
  QSerialPort* m_serialPort;
  QBitArray m_pendingMessagesMask;
  QList<message_hdr_t*>* m_messagesQueue;
  buffer_status_t m_bufferStatus;

  status_hdr_t m_deviceStatus;
  QList<fileheader_data_t>* m_fileList;


  bool m_fileHeaderSent;
  uint64_t  m_totalDataSize;
  uint64_t  m_chunksCount;
  uint64_t  m_chunkIndex;

  bool trySetMessageId(message_hdr_t* message);

  void sendMessage(message_hdr_t* message, uint8_t* data);

  void sendMessageRequest(message_hdr_t* message, uint8_t* data);

  void sendMessageResponse(message_hdr_t* message, uint8_t* data);

  void handleMessageRequest(message_hdr_t* message);

  void handleMessageResponse(message_hdr_t* message);

  void processMessageResponse(message_hdr_t *message);

  void sendStatusResponse(message_hdr_t *request, status_id_t status);

  void sendFakeDeviceStatus(message_hdr_t *request);

  void sendFakeChunkResponse(message_hdr_t *request);

  void processInfoStatusResponse(message_hdr_t *response);

  void processSendFileChunkResponse(message_hdr_t *response);

private slots:
  void readSerialData();

  void handleSerialError(QSerialPort::SerialPortError bufferError);

  void onBytesWritten(qint64 bytes);

  void readMessageFromBuffer();

  void handleMessageError(message_error_t messageError);

  void handleBufferError(buffer_status_t bufferError);

  void handleBufferStatusChanged(buffer_status_t status);

  void processFileSend();

  void handshakeResponseTimeout();

  void infoStatusResponseTimeout();

  void sendCommandResponseTimeout();

  void sendFileResponseTimeout();

  void processMessagesQueue();


signals:
  void bufferError(buffer_status_t);

  void bufferReadyRead();

  void bufferStatusChanged(buffer_status_t);

  void messageError(message_error_t);

  void handshakeResponse(bool success);

  void infoStatusResponse(bool success, status_hdr_t* deviceStatus,  QList<fileheader_data_t>* fileList);

  void sendCommandResponse(bool success);

  void sendFileHeaderResponse(bool success);

  void sendFileChunkResponse(bool success, uint32_t chunk_id, uint32_t chunksCount);




public slots:

};

#endif // CLIENT_H

