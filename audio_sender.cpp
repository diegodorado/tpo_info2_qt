#include "audio_sender.h"

AudioSender::AudioSender(QObject *parent) :
    QObject(parent)
{
}

void AudioSender::test()
{

    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        qDebug() << "Device name: " << deviceInfo.deviceName();



    QAudioFormat format;
    format.setSampleRate(44100);
    // ... other format parameters
    format.setSampleType(QAudioFormat::SignedInt);

    qDebug() << format.sampleType();

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

    if (!info.isFormatSupported(format))
        format = info.nearestFormat(format);

    qDebug() << format.sampleType();

    QString fileName = "/home/diego/music/8bit.wav"; // QFileDialog::getOpenFileName( this,tr("Open Image"), "", tr("WAV Files (*.wav)"));
    QFile file(fileName);

    wav_hdr_t *wav_hdr = new wav_hdr_t();
    if (file.open(QIODevice::ReadOnly)) {
        qint64 bytes = file.read((char *) wav_hdr, sizeof(wav_hdr_t));
        if (bytes == sizeof(wav_hdr_t)) {
            qDebug() << "read was good";
            qDebug() << "file.size() " << file.size();
            qDebug() << "fileSize : " << wav_hdr->fileSize();
            qDebug() << "dataSize : " << wav_hdr->dataSize();
            qDebug() << "WAV_HDR_CHUNK_ID : " << WAV_HDR_CHUNK_ID;
            qDebug() << "ChunkID : " << wav_hdr->ChunkID;
            qDebug() << "validChunkID : " << wav_hdr->validChunkID();
            qDebug() << "validFormat : " << wav_hdr->validFormat();
            qDebug() << "validSubchunk1ID : " << wav_hdr->validSubchunk1ID();
            qDebug() << "validSubchunk2ID : " << wav_hdr->validSubchunk2ID();
            qDebug() << "isPCM : " << wav_hdr->isPCM();
            qDebug() << "is8bit : " << wav_hdr->is8bit();
            qDebug() << "isMono : " << wav_hdr->isMono();
            qDebug() << "valid : " << wav_hdr->valid();
            qDebug() << "NumChannels : " << wav_hdr->NumChannels;
            qDebug() << "SampleRate : " << wav_hdr->SampleRate;
            qDebug() << "ByteRate : " << wav_hdr->ByteRate;
            qDebug() << "BlockAlign : " << wav_hdr->BlockAlign;
            qDebug() << "BitsPerSample : " << wav_hdr->BitsPerSample;

            int i = 7;

            // pointer to integer and back
            unsigned int v1 = reinterpret_cast<unsigned int>(&i); // static_cast is an error
            qDebug() << "The value of &i is 0x" << v1 << "\n";
            int* p1 = reinterpret_cast<int*>(v1);
            qDebug() <<  (p1 == &i);


            // type aliasing through pointer
            char* p2 = reinterpret_cast<char*>(&i);
            if(p2[0] == '\x7')
                qDebug() << "This system is little-endian\n";
            else
                qDebug() << "This system is big-endian\n";

        }
    }
}

void AudioSender::play()
{
    sourceFile.setFileName("/home/diego/music/8bit.wav");
    sourceFile.open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(11025);
    format.setChannelCount(1);
    format.setSampleSize(8);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioOutput(format, this);
    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    audio->start(&sourceFile);
}

void AudioSender::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            audio->stop();
            sourceFile.close();
            delete audio;
            break;

        case QAudio::StoppedState:
            // Stopped for other reasons
            if (audio->error() != QAudio::NoError) {
                // Error handling
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}
