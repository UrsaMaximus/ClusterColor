#include "imageviewer.h"
#include "ui_imageviewer.h"
#include <QScroller>
#include <math.h>


ImageViewer::ImageViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageViewer)
{
    ui->setupUi(this);
    connect(ui->baseImage, SIGNAL(imageClicked(QPointF)), this, SLOT(imageClicked(QPointF)));
    connect(ui->baseImage, SIGNAL(imageWheelZoomed(QPointF,int)), this, SLOT(onImageWheelZoomed(QPointF,int)));

    ui->baseImage->move(0,0);
    ui->overlayImage->move(0,0);
	ui->overlayImage->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    selectionAnimationTimer = new QTimer(this);
    selectionAnimProgress = 0;
    connect(selectionAnimationTimer, SIGNAL(timeout()), this, SLOT(animateSelection()));

    selectionAnimationTimer->setSingleShot(false);
    selectionAnimationTimer->setInterval(32);

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onHScrollChanged(int)));
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onVScrollChanged(int)));

    suspendScrollSync = false;
    zoomLevel = 0;
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

QString ImageViewer::title()
{
    return ui->label->text();
}

void ImageViewer::setTitle(QString title)
{
    ui->label->setText(title);
}

void ImageViewer::animateSelection()
{
    ui->overlayImage->setOpacityF( ( 1.0 - pow(cos(selectionAnimProgress),4)));
    selectionAnimProgress += 0.1;
    while (selectionAnimProgress > M_PI*2.0)
        selectionAnimProgress -= M_PI*2.0;
}

void ImageViewer::imageClicked(QPointF labelLocation)
{
    QPointF imagePixelLocation = labelLocation / GetZoomFactor();
    QPoint rounded(floor(imagePixelLocation.x()), floor(imagePixelLocation.y()));
	emit imageClicked(rounded, this);
}

void ImageViewer::onImageWheelZoomed(QPointF location, int delta)
{
    QPoint viewportLocation = QPoint(location.x() - horizontalScrollBar()->value(),
                                     location.y() - verticalScrollBar()->value());

	emit imageWheelZoomed(viewportLocation, delta);
}

float ImageViewer::GetZoomFactor()
{
    if (zoomLevel > 0)
        return zoomLevel+1;
     else if (zoomLevel < 0)
        return 1.0f / (float)(-zoomLevel+1);
    return 1.0f;
}

// Listen to own scroll bars
void ImageViewer::onHScrollChanged(int hScroll)
{
    if (!suspendScrollSync)
		emit scrollChanged(hScroll, verticalScrollBar()->value());
}

void ImageViewer::onVScrollChanged(int vScroll)
{
    if (!suspendScrollSync)
		emit scrollChanged(horizontalScrollBar()->value(), vScroll);
}

void ImageViewer::EmitScrollEvent()
{
    if (!suspendScrollSync)
		emit scrollChanged(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

// Listen to other image viewers
void ImageViewer::onScrollChanged(int hScroll, int vScroll)
{
    if (!suspendScrollSync)
    {
        horizontalScrollBar()->setValue(hScroll);
        verticalScrollBar()->setValue(vScroll);
    }
}

void ImageViewer::storeRelativeScroll(QPoint pivot)
{
    // Find the pivot point on the image
    float zoom = GetZoomFactor();

    zoomPivot = pivot;
    centerXPx = ( horizontalScrollBar()->value() + zoomPivot.x() ) / zoom;
    centerYPx = ( verticalScrollBar()->value() + zoomPivot.y() ) / zoom;
    suspendScrollSync = true;
}

void ImageViewer::applyRelativeScroll()
{
    // Push the stored center pixel

    float zoom = GetZoomFactor();
    horizontalScrollBar()->setValue(centerXPx * zoom - zoomPivot.x());
    verticalScrollBar()->setValue(centerYPx * zoom - zoomPivot.y());

    suspendScrollSync = false;
}

void ImageViewer::ZoomIn(QPoint pivot)
{
    storeRelativeScroll(pivot);

    zoomLevel+=1;
    if (zoomLevel > 8)
        zoomLevel = 8;
    refreshImageZoom();

    QTimer::singleShot(0, this, SLOT(applyRelativeScroll()));
}

void ImageViewer::ZoomOut(QPoint pivot)
{
    storeRelativeScroll(pivot);

    zoomLevel-=1;
    if (zoomLevel < -8)
        zoomLevel = -8;

    refreshImageZoom();

    QTimer::singleShot(0, this, SLOT(applyRelativeScroll()));
}

void ImageViewer::ZoomIn()
{
    QPoint pivot(ui->scrollArea->width() / 2.0f, ui->scrollArea->height() / 2.0f);
    ZoomIn(pivot);
}

void ImageViewer::ZoomOut()
{
    QPoint pivot(ui->scrollArea->width() / 2.0f, ui->scrollArea->height() / 2.0f);
    ZoomOut(pivot);
}

void ImageViewer::Zoom1x()
{
    QPoint pivot(ui->scrollArea->width() / 2.0f, ui->scrollArea->height() / 2.0f);
    storeRelativeScroll(pivot);

    zoomLevel = 0;
    refreshImageZoom();

    QTimer::singleShot(0, this, SLOT(applyRelativeScroll()));
}

void ImageViewer::SetImage(const QImage& image)
{
    image1x = std::make_unique<QPixmap>();
    image1x->convertFromImage(image);
    refreshImageZoom();
}

void ImageViewer::ClearImage()
{
    image1x = nullptr;
    refreshImageZoom();
}

void ImageViewer::SetSelectionOverlay(const QImage& image)
{
    selectionAnimProgress = 0;
    selectionAnimationTimer->start();
    overlay1x = std::make_unique<QPixmap>();
    overlay1x->convertFromImage(image);
    refreshImageZoom();
}

void ImageViewer::ClearSelectionOverlay()
{
    selectionAnimationTimer->stop();
    overlay1x = nullptr;
    refreshImageZoom();
}

void ImageViewer::refreshImageZoom()
{
    float zoom = GetZoomFactor();
    if (image1x != nullptr)
    {
        ui->baseImage->setVisible(true);
        ui->baseImage->setPixmap(*image1x);
        ui->baseImage->setScaledContents(true);
        ui->baseImage->setFixedSize(zoom*image1x->width(), zoom*image1x->height());
        ui->scrollArea->widget()->setFixedSize(zoom*image1x->width(), zoom*image1x->height());
    }
    else
    {
        ui->baseImage->setVisible(false);
    }

    if (overlay1x != nullptr)
    {
        ui->overlayImage->setVisible(true);
        ui->overlayImage->setPixmap(*overlay1x);
        ui->overlayImage->setScaledContents(true);
        ui->overlayImage->setFixedSize(zoom*overlay1x->width(), zoom*overlay1x->height());
    }
    else
    {
         ui->overlayImage->setVisible(false);
    }
}

QScrollBar* ImageViewer::horizontalScrollBar()
{
    return ui->scrollArea->horizontalScrollBar();
}
QScrollBar* ImageViewer::verticalScrollBar()
{
    return ui->scrollArea->verticalScrollBar();
}
