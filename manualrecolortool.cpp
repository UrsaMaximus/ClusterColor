#include "manualrecolortool.h"
#include "ui_manualrecolortool.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

ManualRecolorTool::ManualRecolorTool(std::map<QString, std::shared_ptr<QImage>> indexImages,
									 std::shared_ptr<QImage> originalPalette,
									 std::shared_ptr<QImage> recolorPalette,
									 QWidget *parent):
	QMainWindow(parent),
	ui(new Ui::ManualRecolorTool),
	indexImages(indexImages),
	originalPalette(originalPalette),
	recolorPalette(recolorPalette)
{
	ui->setupUi(this);

	maxUndoSteps = 16;
	lastStepIndex = -1;
	undoHistory.resize(maxUndoSteps);

	ui->menuImages->clear();
	selectedImageActionGroup = new QActionGroup(this);
	selectedImageActionGroup->setExclusive(true);
	connect(selectedImageActionGroup, SIGNAL(triggered(QAction *)), this, SLOT(on_changedSelectedFile(QAction *)));

	for (auto& imgPair : indexImages)
	{
		QAction* fileSelectAction = new QAction(imgPair.first, selectedImageActionGroup);
		fileSelectAction->setCheckable(true);
		ui->menuImages->addAction(fileSelectAction);
		if (selectedImage.isEmpty())
		{
			selectedImage = imgPair.first;
			fileSelectAction->setChecked(true);
		}
	}

	changeTool(ToolMode::Draw);

	if (originalPalette != nullptr)
	{
		ui->originalPalette->SetImage(*originalPalette);
		ui->originalPalette->ZoomFit();
		ui->originalPalette->maxZoom = 32;
		ui->originalPalette->minZoom = 2;
	}

	if (recolorPalette != nullptr)
	{
		ui->recolorPalette->SetImage(*recolorPalette);
		ui->recolorPalette->ZoomFit();
		ui->recolorPalette->maxZoom = 32;
		ui->recolorPalette->minZoom = 2;
	}

	ui->originalImageViewer->setTitle("Original");
	ui->recolorImageViewer->setTitle("Recolor");
	ui->originalPalette->setTitle("Original Palette");
	ui->recolorPalette->setTitle("Recolor Palette");

	ui->originalImageViewer->setCursor(Qt::CrossCursor);
	ui->recolorImageViewer->setCursor(Qt::CrossCursor);
	ui->originalPalette->setCursor(Qt::CrossCursor);
	ui->recolorPalette->setCursor(Qt::CrossCursor);

	// Link together all the image view scroll bars
	connect(ui->originalImageViewer, SIGNAL(scrollChanged(int,int)), ui->recolorImageViewer, SLOT(onScrollChanged(int,int)));
	connect(ui->recolorImageViewer, SIGNAL(scrollChanged(int,int)), ui->originalImageViewer, SLOT(onScrollChanged(int,int)));
	connect(ui->originalPalette, SIGNAL(scrollChanged(int,int)), ui->recolorPalette, SLOT(onScrollChanged(int,int)));
	connect(ui->recolorPalette, SIGNAL(scrollChanged(int,int)), ui->originalPalette, SLOT(onScrollChanged(int,int)));

	// Connect up the click image signals
	connect(ui->originalImageViewer, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));
	connect( ui->recolorImageViewer, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));
	connect(ui->originalPalette, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));
	connect( ui->recolorPalette, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));

	// Connect up the wheel zoom signals
	connect(ui->originalImageViewer, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomed(QPoint, int)));
	connect( ui->recolorImageViewer, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomed(QPoint, int)));
	connect(ui->originalPalette, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomedPalette(QPoint, int)));
	connect( ui->recolorPalette, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomedPalette(QPoint, int)));

	// Create the color picker and connect its signals
	connect(ui->selectedColorSwatch, SIGNAL(swatchClicked(ClickableSwatch*)), this, SLOT(swatchClicked(ClickableSwatch*)));
	colorPicker = new QColorDialog(this);
	colorPicker->setModal(true);
	connect(colorPicker, SIGNAL(colorSelected(const QColor&)), this, SLOT(on_newColorPreview(const QColor&)));
	connect(colorPicker, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(on_newColorConfirmed(const QColor&)));
	connect(colorPicker, SIGNAL(rejected()), this, SLOT(on_newColorRejected()));
	on_newColorConfirmed(recolorPalette->pixelColor(0,0));

	updateOriginalDisplayEnable = true;
	updateRecolorDisplayEnable = true;
	updateOriginalDisplay();
	updateRecolorDisplay();
	ui->originalPalette->ZoomFit();
	ui->recolorPalette->ZoomFit();
}

void ManualRecolorTool::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   //ui->originalPalette->ZoomFit();
   //ui->recolorPalette->ZoomFit();
}

void ManualRecolorTool::updateSwatchColor(const QColor& color)
{
	QPalette palRecolor = QMainWindow::palette();
	palRecolor.setColor(QPalette::Window, color);
	ui->selectedColorSwatch->setPalette(palRecolor);
	ui->selectedColorSwatch->setAutoFillBackground(true);
}

void ManualRecolorTool::showPicker()
{
	colorPicker->setCurrentColor(selectedColor);
	colorPicker->show();
}

void ManualRecolorTool::swatchClicked(ClickableSwatch*)
{
	showPicker();
}

void ManualRecolorTool::on_newColorRejected()
{
	updateSwatchColor(selectedColor);
}

void ManualRecolorTool::on_newColorPreview(const QColor& color)
{
	updateSwatchColor(color);
}

void ManualRecolorTool::on_newColorConfirmed(const QColor& color)
{
	selectedColor = color;
	updateSwatchColor(selectedColor);
}

void ManualRecolorTool::on_changedSelectedFile(QAction * action)
{
	// Update original and recolor displays
	selectedImage = action->text();
	updateOriginalDisplay();
	updateRecolorDisplay();
}

static QColor samplePalette(const QColor& indexColor, const QImage& palette)
{
  if (indexColor.alpha() == 255)
  {
	  int i = indexColor.red() | (indexColor.green() << 8) | (indexColor.blue() << 16);
	  int x =  i % palette.width();
	  int y = i / palette.width();
	  if (x < palette.width() && y < palette.height())
	  {
		  return palette.pixelColor(x,y);
	  }
  }
  return QColor(0,0,0,0);
}

static std::shared_ptr<QImage> colorIndexImage(const QImage& indexImage, const QImage& palette)
{
	// Given the index image, look up the palette location for each pixel,
	// sample for that location in the palette, and generate a new image

	std::unique_ptr<QImage> recolorImage = std::make_unique<QImage>(indexImage.width(), indexImage.height(), QImage::Format::Format_ARGB32);
	recolorImage->fill(QColor(0,0,0,0));

	for (int y=0; y < indexImage.height(); y++)
	{
		for (int x=0; x < indexImage.width(); x++)
		{
			QColor indexColor = indexImage.pixelColor(x,y);
			QColor mappedColor = samplePalette(indexColor, palette);
			recolorImage->setPixelColor(x, y, mappedColor);
		}
	}
	return recolorImage;
}

void ManualRecolorTool::updateOriginalDisplayInternal()
{
	auto idx = getSelectedIndexImage();
	if (idx != nullptr && originalPalette != nullptr)
	{
		auto remapImage = colorIndexImage(*idx, *originalPalette);
		ui->originalImageViewer->SetImage(*remapImage);
	}
	else
	{
		ui->originalImageViewer->ClearImage();
	}
	updateOriginalDisplayEnable = true;
}

void ManualRecolorTool::updateRecolorDisplayInternal()
{
	ui->recolorPalette->SetImage(*recolorPalette);
	auto idx = getSelectedIndexImage();
	if (idx != nullptr && recolorPalette != nullptr)
	{
		auto remapImage = colorIndexImage(*idx, *recolorPalette);
		ui->recolorImageViewer->SetImage(*remapImage);
	}
	else
	{
		ui->recolorImageViewer->ClearImage();
	}
	updateRecolorDisplayEnable = true;
}

void ManualRecolorTool::updateOriginalDisplay()
{
	if (updateOriginalDisplayEnable)
	{
		updateOriginalDisplayEnable = false;
		QTimer::singleShot(16, this, SLOT(updateOriginalDisplayInternal()));
	}
}

void ManualRecolorTool::updateRecolorDisplay()
{
	if (updateRecolorDisplayEnable)
	{
		updateRecolorDisplayEnable = false;
		QTimer::singleShot(16, this, SLOT(updateRecolorDisplayInternal()));
	}
}

std::shared_ptr<QImage> ManualRecolorTool::getSelectedIndexImage()
{
	if (indexImages.find(selectedImage) == indexImages.end())
		return nullptr;

	return indexImages[selectedImage];
}

void ManualRecolorTool::imageWheelZoomed(QPoint viewportLocation, int delta)
{
	if (delta < 0)
	{
		ui->originalImageViewer->ZoomIn(viewportLocation);
		ui->recolorImageViewer->ZoomIn(viewportLocation);
	}
	else if (delta > 0)
	{
		ui->originalImageViewer->ZoomOut(viewportLocation);
		ui->recolorImageViewer->ZoomOut(viewportLocation);
	}
}

void ManualRecolorTool::imageWheelZoomedPalette(QPoint viewportLocation, int delta)
{
	if (delta < 0)
	{
		ui->originalPalette->ZoomIn(viewportLocation);
		ui->recolorPalette->ZoomIn(viewportLocation);
	}
	else if (delta > 0)
	{
		ui->originalPalette->ZoomOut(viewportLocation);
		ui->recolorPalette->ZoomOut(viewportLocation);
	}
}

static void replaceColorInImage(QImage& matchImage, const QColor& matchColor, QImage& replaceImage, const QColor& replaceColor)
{
	for (int y=0; y < matchImage.height(); y++)
	{
		for (int x=0; x < matchImage.width(); x++)
		{
			if (matchImage.pixelColor(x,y) == matchColor)
			{
				replaceImage.setPixelColor(x,y,replaceColor);
			}
		}
	}
}

void ManualRecolorTool::imageClicked(QPoint pixelLocation, ImageViewer* viewer)
{
	if (toolMode == ToolMode::Draw)
	{
		// If the user clicked the images, sample the color at the coords on the original
		// find that swatch in the original palette, then write the selected color to the recolor palette
		if (viewer == ui->originalImageViewer || viewer == ui->recolorImageViewer)
		{
			auto indexColor = getSelectedIndexImage()->pixelColor(pixelLocation);
			if (indexColor.isValid() && indexColor.alpha() == 255)
			{
				int i = indexColor.red() | (indexColor.green() << 8) | (indexColor.blue() << 16);
				int x =  i % originalPalette->width();
				int y = i / originalPalette->width();
				if (x < recolorPalette->width() && y < recolorPalette->height())
				{
					recolorPalette->setPixelColor(x,y,selectedColor);
					updateRecolorDisplay();
				}
			}
		}
		else if (viewer == ui->originalPalette || viewer == ui->recolorPalette)
		{
			recolorPalette->setPixelColor(pixelLocation,selectedColor);
			updateRecolorDisplay();
		}
	}
	else if (toolMode == ToolMode::Fill)
	{
		QColor matchColor;
		int paletteX = -1;
		int paletteY = -1;

		// Get the palette coords for the color to replace
		if (viewer == ui->originalImageViewer || viewer == ui->recolorImageViewer)
		{
			auto indexColor = getSelectedIndexImage()->pixelColor(pixelLocation);
			if (indexColor.isValid() && indexColor.alpha() == 255)
			{
				int i = indexColor.red() | (indexColor.green() << 8) | (indexColor.blue() << 16);
				paletteX =  i % originalPalette->width();
				paletteY = i / originalPalette->width();
			}
		}
		else
		{
			paletteX = pixelLocation.x();
			paletteY = pixelLocation.y();
		}

		// Get the color to replace
		if (viewer == ui->originalImageViewer || viewer == ui->originalPalette)
		{
			if (paletteX >= 0 && paletteY >= 0 && paletteX < originalPalette->width() && paletteY < originalPalette->height())
				matchColor = originalPalette->pixelColor(paletteX,paletteY);
		}
		else
		{
			if (paletteX >= 0 && paletteY >= 0 && paletteX < recolorPalette->width() && paletteY < recolorPalette->height())
				matchColor = recolorPalette->pixelColor(paletteX,paletteY);
		}

		// Replace the color
		if (matchColor.isValid())
		{
			if (viewer == ui->originalImageViewer || viewer == ui->originalPalette)
				replaceColorInImage(*originalPalette, matchColor, *recolorPalette, selectedColor);
			else
				replaceColorInImage(*recolorPalette, matchColor, *recolorPalette, selectedColor);

			updateRecolorDisplay();
		}
	}
	else if (toolMode == ToolMode::Sample)
	{
		if (viewer == ui->originalImageViewer)
		{
			auto indexColor = getSelectedIndexImage()->pixelColor(pixelLocation);
			if (indexColor.isValid() && indexColor.alpha() == 255)
			{
				int i = indexColor.red() | (indexColor.green() << 8) | (indexColor.blue() << 16);
				int x =  i % originalPalette->width();
				int y = i / originalPalette->width();
				if (x < originalPalette->width() && y < originalPalette->height())
				{
					selectedColor = originalPalette->pixelColor(x,y);
					updateSwatchColor(selectedColor);
				}
			}
		}
		else if (viewer == ui->recolorImageViewer)
		{
			auto indexColor = getSelectedIndexImage()->pixelColor(pixelLocation);
			if (indexColor.isValid() && indexColor.alpha() == 255)
			{
				int i = indexColor.red() | (indexColor.green() << 8) | (indexColor.blue() << 16);
				int x =  i % originalPalette->width();
				int y = i / originalPalette->width();
				if (x < originalPalette->width() && y < originalPalette->height())
				{
					selectedColor = recolorPalette->pixelColor(x,y);
					updateSwatchColor(selectedColor);
				}
			}
		}
		else if (viewer == ui->originalPalette)
		{
			selectedColor = originalPalette->pixelColor(pixelLocation);
			updateSwatchColor(selectedColor);
		}
		else if (viewer == ui->recolorPalette)
		{
			selectedColor = recolorPalette->pixelColor(pixelLocation);
			updateSwatchColor(selectedColor);
		}
	}
	else if (toolMode == ToolMode::Erase)
	{
		if (viewer == ui->originalImageViewer || viewer == ui->recolorImageViewer)
		{
			auto indexColor = getSelectedIndexImage()->pixelColor(pixelLocation);
			if (indexColor.isValid() && indexColor.alpha() == 255)
			{
				int i = indexColor.red() | (indexColor.green() << 8) | (indexColor.blue() << 16);
				int x =  i % originalPalette->width();
				int y = i / originalPalette->width();
				if (x < recolorPalette->width() && y < recolorPalette->height())
				{
					recolorPalette->setPixelColor(x,y,originalPalette->pixelColor(x,y));
					updateRecolorDisplay();
				}
			}
		}
		else if (viewer == ui->originalPalette || viewer == ui->recolorPalette)
		{
			recolorPalette->setPixelColor(pixelLocation,originalPalette->pixelColor(pixelLocation));
			updateRecolorDisplay();
		}
	}
}

ManualRecolorTool::~ManualRecolorTool()
{
	delete ui;
}

void ManualRecolorTool::changeTool(ToolMode mode)
{
	toolMode = mode;
	ui->drawButton->setChecked(mode == ToolMode::Draw);
	ui->sampleButton->setChecked(mode == ToolMode::Sample);
	ui->eraseButton->setChecked(mode == ToolMode::Erase);
	ui->fillButton->setChecked(mode == ToolMode::Fill);
	ui->actionDraw->setChecked(mode == ToolMode::Draw);
	ui->actionSample->setChecked(mode == ToolMode::Sample);
	ui->actionErase->setChecked(mode == ToolMode::Erase);
	ui->actionFill->setChecked(mode == ToolMode::Fill);
}

void ManualRecolorTool::on_actionDraw_triggered()
{
	changeTool(ToolMode::Draw);
}

void ManualRecolorTool::on_actionSample_triggered()
{
	changeTool(ToolMode::Sample);
}

void ManualRecolorTool::on_actionErase_triggered()
{
	changeTool(ToolMode::Erase);
}

void ManualRecolorTool::on_actionFill_triggered()
{
	changeTool(ToolMode::Fill);
}

void ManualRecolorTool::on_actionColor_triggered()
{
	swatchClicked(ui->selectedColorSwatch);
}


void ManualRecolorTool::on_actionSave_Recolor_Palette_triggered()
{
	QString recolorPaletteName = QString("Recolor.png");
	QFileInfo selectedImageInfo(selectedImage);
	QDir selectedImageDir = selectedImageInfo.dir();
	QString recolorPath = selectedImageDir.filePath(recolorPaletteName);

	QString filename = QFileDialog::getSaveFileName(this, tr("Save Recolor Palette"), recolorPath, tr("Image Files (*.png)"));

	if (!filename.isNull())
	{
		try
		{
			recolorPalette->save(filename);
		}
		catch (std::runtime_error& err)
		{
			QMessageBox msgBox;
			msgBox.setText(err.what());
			msgBox.exec();
			return;
		}
	}
}


void ManualRecolorTool::on_actionOpen_Recolor_Palette_triggered()
{
	QString paletteFileName = QFileDialog::getOpenFileName(this, tr("Open Recolor Palette"), "", tr("Image Files (*.png)"));

	if (!paletteFileName.isNull())
	{
		recolorPalette = std::make_shared<QImage>(paletteFileName);
		updateRecolorDisplay();
	}
}


//std::vector<std::shared_ptr<QImage>> undoHistory;
//int maxUndoSteps;
//int lastStepIndex;
void ManualRecolorTool::undo()
{
	if (lastStepIndex >= 0 && lastStepIndex < maxUndoSteps && undoHistory[lastStepIndex] != nullptr)
	{
		recolorPalette = undoHistory[lastStepIndex];
		lastStepIndex--;
	}
}

void ManualRecolorTool::redo()
{
	if ((lastStepIndex+1) >= 0 && (lastStepIndex+1) < maxUndoSteps && undoHistory[(lastStepIndex+1)] != nullptr)
	{
		recolorPalette = undoHistory[(lastStepIndex+1)];
		lastStepIndex++;
	}
}

void ManualRecolorTool::commitToUndoHistory()
{
	// Make room in the collection if needed
}
