#ifndef CLICKABLESWATCH_H
#define CLICKABLESWATCH_H

#include <QFrame>

class ClickableSwatch : public QFrame
{
    Q_OBJECT
public:
    ClickableSwatch(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void swatchClicked(ClickableSwatch* swatch);
};

#endif // CLICKABLESWATCH_H
