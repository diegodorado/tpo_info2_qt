#ifndef AUDIO_SENDER_H
#define AUDIO_SENDER_H

#include <QObject>

#include <QDebug>
#include "wav.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QFileDialog>
#include <QTimer>

#include "audio_buffer.h"

class AudioSender : public QObject
{
    Q_OBJECT
    void initAudio();
    void initBuffer();
    void initTimer();
    void initFile();
    QAudioFormat m_format;
    QFile m_file;
    QAudioOutput* m_audio;
    QByteArray m_buffer_data;
    QBuffer* m_buffer;
    QTimer *m_timer;



public:

    explicit AudioSender(QObject *parent = 0);

    void test();
    void play();
    void load();
signals:

public slots:

    void handleStateChanged(QAudio::State newState);
    void handleReadyRead();
    void handleTimerTimeout();
};

#endif // AUDIO_SENDER_H
