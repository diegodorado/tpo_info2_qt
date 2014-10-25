#ifndef AUDIO_SENDER_H
#define AUDIO_SENDER_H

#include <QObject>

#include <QDebug>
#include "wav.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QFileDialog>


class AudioSender : public QObject
{
    Q_OBJECT
public:

    QFile sourceFile;   // class member.
    QAudioOutput* audio; // class member.

    explicit AudioSender(QObject *parent = 0);

    void test(void);
    void play(void);

signals:

public slots:

    void handleStateChanged(QAudio::State newState);
};

#endif // AUDIO_SENDER_H
