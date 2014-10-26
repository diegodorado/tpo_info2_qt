#include "audio_sender.h"

AudioSender::AudioSender(QObject *parent) :
  QObject(parent)
{
  // Set up the format, eg.
  m_format.setSampleRate(11025);
  m_format.setChannelCount(1);
  m_format.setSampleSize(8);
  m_format.setCodec("audio/pcm");
  m_format.setByteOrder(QAudioFormat::LittleEndian);
  m_format.setSampleType(QAudioFormat::UnSignedInt);

}


void AudioSender::initBuffer(){

  m_buffer = new QBuffer(&m_buffer_data);
  m_buffer->open(QIODevice::ReadWrite);
}

void AudioSender::initFile(){
  m_file.setFileName("/home/diego/music/8bit.wav");
  m_file.open(QIODevice::ReadOnly);
}

void AudioSender::initAudio(){
  m_audio = new QAudioOutput(m_format, this);
  m_audio->setBufferSize(4096);
  connect(m_audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
  m_audio->start(m_buffer);

}

void AudioSender::initTimer(){
  m_timer = new QTimer();
  connect(m_timer, SIGNAL(timeout()), this, SLOT(handleTimerTimeout()));
  m_timer->start(1000);
}


void AudioSender::handleStateChanged(QAudio::State newState)
{
  qDebug() << "AudioSender::handleStateChanged: " << newState ;

  switch (newState) {
    case QAudio::IdleState:
      m_buffer->seek(0);
      // Finished playing (no more data)
      break;

    case QAudio::StoppedState:
      // Stopped for other reasons
      if (m_audio->error() != QAudio::NoError) {
        // Error handling
        qDebug() << "m_audio->error(): " << m_audio->error();

      }
      break;

    default:
      // ... other cases as appropriate
      break;
  }
}

void AudioSender::load(){
  initBuffer();
  initAudio();
  initFile();
  initTimer();
}


void AudioSender::handleTimerTimeout()
{

  int chunks = m_audio->bytesFree()/m_audio->periodSize();

  //qDebug() << "bytesFree: " << m_audio->bytesFree() << "periodSize: " << m_audio->periodSize() ;
  while (chunks-- && (m_file.bytesAvailable () >= m_audio->periodSize()) ){
    qDebug() << m_file.bytesAvailable ();
    m_buffer_data.append(m_file.read(m_audio->periodSize()));
  }


}
