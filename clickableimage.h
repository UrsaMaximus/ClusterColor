#ifndef CLICKABLEIMAGE_H
#define CLICKABLEIMAGE_H

#include <QLabel>

class ClickableImage : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableImage(QWidget *parent = nullptr);

    // Set how opaque this image is. 0.0 = transparent, 1.0 = opaque
    void setOpacityF(float opacity);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent * event) override;

signals:
    void imageClicked(QPointF localPosition);
    void imageWheelZoomed(QPointF localPosition, int zoomDelta);

private:
    float _opacity;
    QPoint _scrollDelta;

};

#endif // CLICKABLEIMAGE_H
