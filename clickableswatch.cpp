#include "clickableswatch.h"

ClickableSwatch::ClickableSwatch(QWidget *parent) : QFrame(parent)
{

}

void ClickableSwatch::mousePressEvent(QMouseEvent *event)
{

}
void ClickableSwatch::mouseReleaseEvent(QMouseEvent *event)
{
    swatchClicked(this);
}
