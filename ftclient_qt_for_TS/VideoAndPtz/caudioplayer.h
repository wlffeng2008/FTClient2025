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
    virtual ~CAudioPlayer(){m_bExit = true ;};

    void forceExit(){m_bExit = true ;};
    void pushBuf(const QByteArray&buf) ;

protected:
    void run() override;

    QByteArrayList m_ABufs ;
    QMutex m_Mutex;
    bool m_bExit = false ;
};

#endif // CAUDIOPLAYER_H
