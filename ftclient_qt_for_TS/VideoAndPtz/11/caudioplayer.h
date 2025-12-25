#ifndef CAUDIOPLAYER_H
#define CAUDIOPLAYER_H

#include <QObject>
#include <QThread>

#include <QMutex>
#include <QByteArrayList>

class CAudioPlayer : public QThread
{
    Q_OBJECT
public:
    explicit CAudioPlayer(QObject *parent = nullptr);

    void pushBuf(const QByteArray&buf) ;

protected:
    void run() override;

    QByteArrayList m_ABufs ;
    QMutex m_Mutex;
};

#endif // CAUDIOPLAYER_H
