#ifndef FRAMEPRINTCONTROL_H
#define FRAMEPRINTCONTROL_H

#include "CustomItems.h"

#include <QFrame>
#include <QSettings>
#include <QTimer>
#include <QMap>

namespace Ui {
class FramePrintControl;
}

typedef struct
{
    QString strMac ;
    QString strDID ;
    QString strDate ;
    QString strSN ;
    QString strQPass ;
}DataItem;

class FrameLabelView ;

class FramePrintControl : public QFrame
{
    Q_OBJECT

public:
    explicit FramePrintControl(QWidget *parent = nullptr);
    ~FramePrintControl();
    void BindLabelView(FrameLabelView *pView);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_pushButtonPreview_clicked();
    void on_pushButtonPrint_clicked();

private:
    Ui::FramePrintControl *ui;
    QSettings *m_pSet = nullptr;

    CustomTextItem *m_pItemText = nullptr;
    CustomPixmapItem *m_pItemPixmap = nullptr;

    QString m_strConfigFile;
    QString m_strTemplFile;
    void LoadTemplate(const QString&strFile);
    void LoadConfig(const QString&strFile);
    void SaveConfig() ;

    bool m_bCanSet = true;
    void updateItemSize();
    QTimer m_TMCreate ;
    FrameLabelView *m_pLabelView = nullptr;

    QList<DataItem>m_DMap ;
    void LoadMap() ;
};

#endif // FRAMEPRINTCONTROL_H
