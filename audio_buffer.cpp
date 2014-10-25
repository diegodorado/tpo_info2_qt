#include "audio_buffer.h"

AudioBuffer::AudioBuffer()
{
}

void AudioBuffer::start()
{
    open(QIODevice::ReadWrite);
    qDebug() << "audio buffer start\n";
}
