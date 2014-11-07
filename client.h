#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QByteArray>
#include <QBitArray>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include "protocol.h"


class Client : public QObject
{
  Q_OBJECT

public:
  explicit Client(QObject *parent = 0);

  void sendTest(void);

  enum message_error_t {
    NULL_MESSAGE,
    INVALID_MESSAGE_TYPE,
    RESPONSE_NOT_EXPECTED,
    MSG_ID_IN_USE
  };
  void sendHandshakeRequest();
  void sendStatusResponse(message_t *request, status_id_t status);

private:
  QTimer* m_queueTimer;
  QFile m_audioFile;
  QSerialPort* m_serialPort;
  QBitArray m_pendingMessagesMask;
  QList<message_t*>* m_messagesQueue;
  buffer_status_t m_bufferStatus;

  void sendMessage(message_t* message);
  void sendMessageRequest(message_t* message);
  void sendMessageResponse(message_t* message);
  void handleMessageRequest(message_t* message);
  void handleMessageResponse(message_t* message);
  void openSerialPort();
  void closeSerialPort();
  void writeData(const QByteArray &data);
  void sendFileHeader(fileheader_data_t *fileheader_data);

  void sendFileChunk();
private slots:
  void readSerialData();
  void handleSerialError(QSerialPort::SerialPortError bufferError);
  void onBytesWritten(qint64 bytes);
  void readMessageFromBuffer();
  void handleMessageError(message_error_t messageError);
  void handleBufferError(buffer_status_t bufferError);
  void handleBufferStatusChanged(buffer_status_t status);
  void processMessagesQueue();
signals:
  void bufferError(buffer_status_t);
  void bufferReadyRead();
  void bufferStatusChanged(buffer_status_t);
  void messageError(message_error_t);


public slots:

};

#endif // CLIENT_H

