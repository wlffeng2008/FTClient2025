#ifndef HTTPFORTANGE_H
#define HTTPFORTANGE_H

#include <QObject>

class CHttpForTange : public QObject
{
    Q_OBJECT
public:
    explicit CHttpForTange(QObject *parent = nullptr);
    virtual ~CHttpForTange();

signals:
};

#endif // HTTPFORTANGE_H
