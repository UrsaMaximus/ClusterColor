#include "clickableimage.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOption>
#include <QtMath>
#include <QWheelEvent>
#include <QGuiApplication>

ClickableImage::ClickableImage(QWidget *parent) : QLabel(parent)
{
    _opacity = 1.0f;
}

void ClickableImage::setOpacityF(float opacity)
{
    _opacity = std::clamp(opacity, 0.0f, 1.0f);
    update();
}

void ClickableImage::paintEvent(QPaintEvent *)
{
	//QStyle *style = QWidget::style();
    QPainter painter(this);

    drawFrame(&painter);
    QRect cr = contentsRect();
    cr.adjust(margin(), margin(), -margin(), -margin());

	//int align = QStyle::visualAlignment(layoutDirection(), QFlag(alignment()));

	if (!pixmap().isNull())
    {
        painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, false);

        painter.setOpacity(_opacity);
		painter.drawPixmap(cr, pixmap(), pixmap().rect());
    }
}

void ClickableImage::wheelEvent(QWheelEvent * event)
{
	//bool ctrlHeld = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
	if(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))
    {
        // Phaseless (legacy) scrolling just passes through deltas directly
        if (event->phase() == Qt::ScrollPhase::NoScrollPhase && event->source() == Qt::MouseEventNotSynthesized)
        {
			emit imageWheelZoomed(event->position(), event->angleDelta().y());
        }

        // If we have phased scrolling, reset the scroll delta on begin and end
        else if (event->phase() == Qt::ScrollPhase::ScrollBegin || event->phase() == Qt::ScrollPhase::ScrollEnd)
        {
            _scrollDelta = QPoint();
        }

        else if (event->phase() == Qt::ScrollPhase::ScrollUpdate)
        {
            // If we don't have a pixel delta at all, pass through the deltas directly
            if (event->pixelDelta().isNull() && event->source() == Qt::MouseEventNotSynthesized)
            {
				emit imageWheelZoomed(event->position(), event->angleDelta().y());
            }
            else if (!event->pixelDelta().isNull())
            {
                // Accumulate scroll deltas until we hit +48 or -48 then trigger an event
                _scrollDelta += event->pixelDelta();

                if (_scrollDelta.y() > 48)
                {
					emit imageWheelZoomed(event->position(), 1);
                    _scrollDelta = QPoint();
                }
                else if (_scrollDelta.y() < -48)
                {
					emit imageWheelZoomed(event->position(), -1);
                    _scrollDelta = QPoint();
                }
            }
        }

        event->accept();
    }
	else
	{
		event->ignore();
	}
}

void ClickableImage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
		emit imageClicked(event->position());
}
void ClickableImage::mouseReleaseEvent(QMouseEvent *)
{

}
