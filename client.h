#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QByteArray>
#include <QBitArray>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QtSerialPort/QSerialPort>
#include "protocol.h"


class Client : public QObject
{
  Q_OBJECT

public:
  explicit Client(QObject *parent = 0);
  ~Client();

  QSerialPort *getSerialPort(void);

  bool openSerialPort(QString port, uint16_t baudRate);

  void closeSerialPort(void);

  void sendHandshakeRequest();

  void sendCommandRequest(command_type_t command);

  void getDeviceStatus();

  void sendFile(QFile *file, uint32_t sampleRate, QString filename);

private:
  const int MAX_CONCURRENT_MESSAGES = 16;
  QTimer* m_fileSendTimer;
  QTimer* m_keepAliveTimer;
  QTimer* m_deadLineTimer;


  QFile* m_audioFile;
  QSerialPort* m_serialPort;
  QBitArray m_pendingMessagesMask;
  buffer_status_t m_bufferStatus;

  status_hdr_t* m_deviceStatus;
  QList<QString>* m_fileList;

  int m_deviceConnected;
  bool m_fileHeaderSent;
  bool m_fileHeaderAcepted;
  fileheader_data_t m_fileHeader;
  uint32_t  m_chunkIndex;

  bool pendingFull();

  bool canSendMessage();

  void sendMessage(message_hdr_t* message, uint8_t* data);

  void sendMessageRequest(message_hdr_t* message, uint8_t* data);

  void sendMessageResponse(message_hdr_t* message, uint8_t* data);

  void processMessageRequest(message_hdr_t *message);

  void processMessageResponse(message_hdr_t *message);

  void sendStatusResponse(message_hdr_t *request, status_id_t status);

  void sendFakeDeviceStatus(message_hdr_t *request);

  void sendFakeChunkResponse(message_hdr_t *request);

  void processInfoStatusResponse(message_hdr_t *response);

  void processSendFileChunkResponse(message_hdr_t *response);

  void readMessageFromBuffer();

  void updateDeviceStatus(bool connected);

  void finishOrCancelFileTransfer(void);


private slots:
  void readSerialData();

  void processFileSend();

  void keepAlive();

  void deadLine();


signals:

  void deviceStatusChanged(bool connected);

  void infoStatusResponse(bool success, status_hdr_t* deviceStatus,  QList<QString>* fileList);

  void sendCommandResponse(bool success);

  void sendFileHeaderResponse(bool success);

  void sendFileChunkResponse(bool success, uint32_t chunk_id, uint32_t chunksCount);

  void log(QString message);




public slots:

};

#endif // CLIENT_H

