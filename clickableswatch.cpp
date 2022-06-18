#include "clickableswatch.h"

ClickableSwatch::ClickableSwatch(QWidget *parent) : QFrame(parent)
{

}

void ClickableSwatch::mousePressEvent(QMouseEvent*)
{

}
void ClickableSwatch::mouseReleaseEvent(QMouseEvent*)
{
	emit swatchClicked(this);
}
