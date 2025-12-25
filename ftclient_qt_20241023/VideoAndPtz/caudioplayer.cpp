#include "caudioplayer.h"
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioSink>
#include <QBuffer>
#include <QCoreApplication>
#include <QIODevice>

CAudioPlayer::CAudioPlayer(QObject *parent)
    : QThread{parent}
{
    start() ;
}

void CAudioPlayer::pushBuf(const QByteArray&buf)
{
    QMutexLocker lock(&m_Mutex);
    if(buf.size()>10)
        m_ABufs.push_back(buf) ;
}

void CAudioPlayer::run()
{
    // 设置音频格式
    QAudioFormat format;
    format.setSampleRate(44100); // G.711通常使用8kHz采样率
    format.setChannelCount(2);  // 单声道
    format.setSampleFormat(QAudioFormat::Int16); // 16位PCM

    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();

    QAudioSink audioSink(audioDevice, format);

    QIODevice *pDevice = audioSink.start();
    audioSink.setVolume(0.99) ;
    audioSink.setBufferSize(32000);

    while(!m_bExit)
    {
        QMutexLocker lock(&m_Mutex);
        if(m_ABufs.isEmpty())
        {
            QCoreApplication::processEvents();
            continue;
        }

        QByteArray tmp = m_ABufs[0];
        m_ABufs.pop_front() ;

        pDevice->write(tmp);
        QThread::usleep(10);
    }
}
