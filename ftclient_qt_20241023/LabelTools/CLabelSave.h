#ifndef CLABELSAVE_H
#define CLABELSAVE_H

#include <QImage>
#include <QByteArray>
#include <QString>
#include <QGraphicsScene>

typedef struct
{
    int width;
    int height;
    int fontSize ;
    bool fontBold;
    QString fontName ;
    QString fontStyle ;
    QString strTempl;
}TemplInfo;


class CLabelSave
{
public:
    explicit CLabelSave();
    virtual ~CLabelSave();

    static QImage decodeImage( const QString &imageData );
    static QByteArray encodeImage( const QImage &image );
    static bool saveSceneWithImages( QGraphicsScene *scene, const QString &filePath,TemplInfo&Info);
    static bool loadSceneWithImages( QGraphicsScene *scene, const QString &filePath,TemplInfo&Info);

};

#endif // CLABELSAVE_H
