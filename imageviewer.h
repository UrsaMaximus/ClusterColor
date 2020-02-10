#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QTimer>

namespace Ui {
class ImageViewer;
}

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();


    void SetImage(const QImage& image);
    void ClearImage();

    void SetSelectionOverlay(const QImage& image);
    void ClearSelectionOverlay();

    void ZoomIn();
    void ZoomOut();
    void ZoomIn(QPoint pivot);
    void ZoomOut(QPoint pivot);
    void Zoom1x();

    void EmitScrollEvent();

    QString title();
    void setTitle(QString);

signals:
    void imageClicked(QPoint, ImageViewer* source);

    // Broadcast to other image viewers
    void scrollChanged(int hScroll, int vScroll);

    void imageWheelZoomed(QPoint, int);

private slots:
    void imageClicked(QPointF imageLocation);
    void animateSelection();
    void applyRelativeScroll();

    // Listen to own scroll bars
    void onHScrollChanged(int hScroll);
    void onVScrollChanged(int vScroll);

    // Listen for wheel zooms
    void onImageWheelZoomed(QPointF,int);

public slots:
    // Listen to other image viewers
    void onScrollChanged(int hScroll, int vScroll);


private:
    Ui::ImageViewer *ui;
    std::unique_ptr<QPixmap> image1x;
    std::unique_ptr<QPixmap> overlay1x;
    QTimer* selectionAnimationTimer;
    float selectionAnimProgress;

    int zoomLevel;
     void refreshImageZoom();
     float GetZoomFactor();
     QScrollBar* horizontalScrollBar();
     QScrollBar* verticalScrollBar();

     // Fancy scroll functions
     float centerXPx, centerYPx;
     QPoint zoomPivot;
     bool suspendScrollSync;
     void storeRelativeScroll(QPoint pivot);


};

#endif // IMAGEVIEWER_H
