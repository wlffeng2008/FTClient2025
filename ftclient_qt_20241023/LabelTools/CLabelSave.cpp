#include "CLabelSave.h"
#include "Label.h"
#include "CustomItems.h"
#include "CLabelEdit.h"
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QBuffer>
#include <QImage>
#include <QGraphicsScene>
#include <QDebug>

CLabelSave::CLabelSave()
{

}

CLabelSave::~CLabelSave()
{

}


QImage CLabelSave::decodeImage(const QString &imageData)
{
    QByteArray byteArray = QByteArray::fromBase64(imageData.toLatin1());
    QImage image;
    image.loadFromData(byteArray, "PNG");  // 从 Base64 编码数据加载图片
    return image;
}

QByteArray CLabelSave::encodeImage( const QImage &image )
{
    QByteArray byteArray;
    QBuffer buffer( &byteArray );
    buffer.open( QIODevice::WriteOnly );
    image.save( &buffer, "PNG" );  // 将图片以 PNG 格式保存到缓冲区
    return byteArray.toBase64();   // 将图片数据进行 Base64 编码
}

bool CLabelSave::saveSceneWithImages(QGraphicsScene *scene, const QString &filePath,TemplInfo&Info)
{
    QJsonArray itemsArray;

    for (auto item : scene->items())// 遍历场景中的所有对象
    {
        if (auto textItem = dynamic_cast<CustomTextItem*>(item))
        {
            QFont tf = textItem->font() ;
            QJsonObject textObject;
            textObject["type"] = "text";
            textObject["text"] = textItem->toPlainText();

            textObject["font"] = tf.family();
            textObject["style"] = tf.styleName();
            textObject["bold"] = tf.bold();
            textObject["size"] = tf.pointSize();

            textObject["x"] = textItem->pos().x();
            textObject["y"] = textItem->pos().y();
            textObject["scale"] = textItem->scale();
            textObject["type_in"] = textItem->data( 0 ).toString();
            itemsArray.append(textObject);
        }
        else if (auto pixmapItem = dynamic_cast<CustomPixmapItem*>(item))
        {
            QJsonObject pixmapObject;
            pixmapObject["type"] = "pixmap";
            pixmapObject["x"] = pixmapItem->pos().x();
            pixmapObject["y"] = pixmapItem->pos().y();
            pixmapObject["scale"] = pixmapItem->scale();
            pixmapObject["type_in"] = pixmapItem->data( 0 ).toString();

            QImage image = pixmapItem->pixmap().toImage();
            QByteArray imageData = encodeImage(image);
            pixmapObject["imageData"] = QString::fromLatin1(imageData);

            itemsArray.append(pixmapObject);
        }
    }

    // 保存到 JSON 文件
    QJsonObject sceneObject;

    QJsonObject jInfo;
    jInfo["width"] = Info.width ;
    jInfo["height"] = Info.height ;
    jInfo["fontsize"] = Info.fontSize ;
    jInfo["fontname"] = Info.fontName ;
    jInfo["fontstyle"] = Info.fontStyle ;
    jInfo["fontbold"] = Info.fontBold ;
    jInfo["qrtempl"] = Info.strTempl ;
    sceneObject["TemplInfo"] = jInfo;

    sceneObject["items"] = itemsArray;

    QFile file( filePath );
    if( file.open( QIODevice::WriteOnly ) )
    {
        QJsonDocument doc( sceneObject );
        if( file.write( doc.toJson() ) < 0 )
        {
            qDebug() << "Label template data write error.";
            file.close();
            return false;
        }
        file.close();
        return true;
    }
     return false;
}

bool CLabelSave::loadSceneWithImages(QGraphicsScene *scene, const QString &filePath,TemplInfo&Info)
{
    if( nullptr == scene )
    {
        qDebug() << "scene is nullptr";
        return false;
    }

    QFile file( filePath );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        qDebug() << "File open error: " << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    QJsonObject sceneObject = doc.object();
    QJsonArray itemsArray = sceneObject["items"].toArray();

    if(sceneObject.contains("TemplInfo"))
    {
        QJsonObject TInfo = sceneObject["TemplInfo"].toObject();
        Info.width = TInfo["width"].toInt();
        Info.height = TInfo["height"].toInt();
        Info.fontSize = TInfo["fontsize"].toInt();
        Info.fontBold = TInfo["fontbold"].toBool();
        Info.fontName = TInfo["fontname"].toString();
        Info.fontStyle = TInfo["fontstyle"].toString();
        Info.strTempl = TInfo["qrtempl"].toString();
    }
    else
    {
        Info.width = 30;
        Info.height = 20;
        Info.fontSize = 9;
        Info.fontBold = true;
        Info.fontName = "Bahnschrift";
        Info.fontStyle = "Condensed";
    }

    scene->clear();

    static QFont font;
    font.setFamily(Info.fontName);
    font.setStyleName(Info.fontStyle);
    font.setPointSize(Info.fontSize);
    font.setBold(Info.fontBold);

    int nW = scene->width();
    int nH = scene->height();

    for( const QJsonValue &value : itemsArray )
    {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString();

        if (type == "text")
        {
            QString text = obj["text"].toString();
            qreal x = obj["x"].toDouble();
            qreal y = obj["y"].toDouble();
            qreal scale = obj["scale"].toDouble();
            QString type_in = obj["type_in"].toString();

            CustomTextItem *textItem = new CustomTextItem( text );
            textItem->setPos(x, y);
            textItem->setScale( scale );
            textItem->setData( 0, type_in );
            textItem->setFont(font) ;
            if(obj.contains("font"))
            {
                QFont font1;
                font1.setFamily(obj["font"].toString());
                font1.setStyleName(obj["style"].toString());
                font1.setBold( obj["bold"].toBool());
                font1.setPointSize(obj["size"].toInt());
                textItem->setFont(font1) ;
            }

            if( scene->parent() )
            {
                CLabelEdit* pParent = (CLabelEdit*)scene->parent();
                QObject::connect( textItem, SIGNAL( sigRectChanged_( QRectF ) ) , pParent, SLOT( onCurItemRectChanged( QRectF ) ) );
            }

            scene->addItem(textItem);
        }
        else if (type == "pixmap")
        {
            qreal x = obj["x"].toDouble();
            qreal y = obj["y"].toDouble();
            qreal scale = obj["scale"].toDouble();
            QString type_in = obj["type_in"].toString();
            QString imageData = obj["imageData"].toString();

            // if(scale <= 0.1)
            //     scale = 0.5;
            if(x<0)
                x=0 ;
            if(y<0)
                y=0 ;

            if(x > nW-10)
                x = nW-10 ;
            if(y > nH-10)
                y = nH-10 ;

            QImage image = decodeImage(imageData);
            CustomPixmapItem *pixmapItem = new CustomPixmapItem(QPixmap::fromImage(image));
            pixmapItem->setPos(x, y);
            pixmapItem->setScale( scale );
            pixmapItem->setData( 0, type_in );

            if( scene->parent() )
            {
                CLabelEdit* pParent = (CLabelEdit*)scene->parent();
                QObject::connect( pixmapItem, SIGNAL( sigRectChanged_( QRectF ) ) , pParent, SLOT( onCurItemRectChanged( QRectF ) ) );
            }

            scene->addItem(pixmapItem);
        }
    }

    return true;
}
