#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <QBuffer>
#include <QDebug>

class AudioBuffer : public QBuffer
{
    Q_OBJECT

public:
    AudioBuffer();
    void start();
};

#endif // AUDIO_BUFFER_H
