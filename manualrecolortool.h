#ifndef MANUALRECOLORTOOL_H
#define MANUALRECOLORTOOL_H

#include <QMainWindow>
#include "clustercolormain.h"

namespace Ui {
class ManualRecolorTool;
}

enum class ToolMode
{
	Draw,
	Sample,
	Erase,
	Fill
};

class ManualRecolorTool : public QMainWindow
{
	Q_OBJECT

public:
	explicit ManualRecolorTool(std::map<QString, std::shared_ptr<QImage>> indexImages,
							   std::shared_ptr<QImage> originalPalette,
							   std::shared_ptr<QImage> recolorPalette,
							   QWidget *parent = nullptr);
	~ManualRecolorTool();

private slots:
	void on_changedSelectedFile(QAction *);
	void on_newColorRejected();
	void on_newColorPreview(const QColor& color);
	void on_newColorConfirmed(const QColor& color);
	void imageWheelZoomed(QPoint viewportLocation, int delta);
	void imageWheelZoomedPalette(QPoint viewportLocation, int delta);
	void imageClicked(QPoint pixelLocation, ImageViewer* source);
	void swatchClicked(ClickableSwatch*);
	void updateOriginalDisplayInternal();
	void updateRecolorDisplayInternal();
	void on_actionDraw_triggered();
	void on_actionSample_triggered();
	void on_actionErase_triggered();
	void on_actionFill_triggered();
	void on_actionColor_triggered();
	void on_actionSave_Recolor_Palette_triggered();
	void on_actionOpen_Recolor_Palette_triggered();

private:
	Ui::ManualRecolorTool *ui;
	QColorDialog* colorPicker;
	QActionGroup* selectedImageActionGroup;
	QString selectedImage;
	QColor selectedColor;
	bool updateOriginalDisplayEnable;
	bool updateRecolorDisplayEnable;
	ToolMode toolMode;
	std::map<QString, std::shared_ptr<QImage>> indexImages;
	std::shared_ptr<QImage> originalPalette;
	std::shared_ptr<QImage> recolorPalette;

	void updateOriginalDisplay();
	void updateRecolorDisplay();
	void updateSwatchColor(const QColor& color);
	void showPicker();
	void resizeEvent(QResizeEvent*);
	void changeTool(ToolMode mode);

	std::vector<std::shared_ptr<QImage>> undoHistory;
	int maxUndoSteps;
	int lastStepIndex;
	void undo();
	void redo();
	void commitToUndoHistory();

	std::shared_ptr<QImage> getSelectedIndexImage();
};

#endif // MANUALRECOLORTOOL_H
