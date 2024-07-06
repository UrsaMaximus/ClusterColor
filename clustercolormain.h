#ifndef CLUSTERCOLORMAIN_H
#define CLUSTERCOLORMAIN_H

#include <QMainWindow>
#include <QActionGroup>
#include "automaticgroupgeneration.h"
#include "palette.h"
#include "palettegroupview.h"
#include "imageviewer.h"
#include "preferences.h"
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class ClusterColorMain; }
QT_END_NAMESPACE

class ClusterColorMain : public QMainWindow
{
    Q_OBJECT

public:
    ClusterColorMain(QWidget *parent = nullptr);
    ~ClusterColorMain();

	// Singletons other forms might want
	Preferences* _prefs;

	// Global document state
	std::unique_ptr<Palette> palette;
	std::map<QString, std::shared_ptr<QImage>> images;

private slots:

    // Group generation slots
    void autoGenerateGroups(int numberOfClusters, bool saveRecolors);
    void createBackupPalette();
    void deleteBackupPalette();
    void restoreBackupPalette();

    void imageClicked(QPoint, ImageViewer* source);

    void imageWheelZoomed(QPoint viewportLocation, int delta);

    void on_changedSelectedFile(QAction *);

    void on_actionOpen_Color_Image_triggered();

    void on_actionExport_Index_Image_triggered();

    void on_actionExport_Recolor_Palette_triggered();

    void on_actionExport_Original_Palette_triggered();

    void on_actionOpen_Palette_as_Recolor_triggered();

    void on_actionOpen_Index_Image_triggered();

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionZoom_Fit_triggered();

    void on_actionShow_Original_Color_toggled(bool arg1);

    void on_actionShow_Control_Map_toggled(bool arg1);

    void on_actionShow_Remapped_Color_toggled(bool arg1);

    void on_actionIsolate_Selected_Color_triggered();

    void on_actionSelect_None_triggered();

    void on_actionEdit_Selected_Color_Group_triggered();

    // Palette Group Editor slots
    void swatchClicked(ClickableSwatch*);
    void on_newColorPreview(const QColor& color);
    void on_newColorConfirmed(const QColor& color);
    void on_newColorRejected();
    void changeSelectedGroup(PaletteGroupView* view);
    void changeSelectedGroup(std::shared_ptr<PaletteGroup> group);

    //void on_paletteGroupChanged(const PaletteGroup& group);
    void on_resetPalette_clicked();

    void onHueSlider_valueChanged(float value);
    void onSaturationSlider_valueChanged(float value);
    void onValueSlider_valueChanged(float value);

    void onHueCompressionSlider_valueChanged(float value);
    void onSaturationCompressionSlider_valueChanged(float value);
    void onValueCompressionSlider_valueChanged(float value);

    void on_singleGroupResetButton_clicked();

    void on_actionExport_All_triggered();

    void on_actionOpen_Original_Palette_triggered();

    void on_actionAppend_Group_with_Selection_triggered();

    void on_actionSelect_All_triggered();

    void on_actionSelect_Inverse_triggered();

    void on_actionReset_Recolors_triggered();

    void on_actionReset_Groups_triggered();

    void on_actionAutomatic_Group_Generation_triggered();

    void on_actionAbout_ClusterColor_triggered();

    void on_opaqueCheckbox_toggled(bool checked);

    void updateOriginalDisplay();
    void updateControlDisplay();
    void updateRecolorDisplay();
    void updateSelectedColorOverlay();
    void updateOriginalDisplayInternal();
    void updateControlDisplayInternal();
    void updateRecolorDisplayInternal();
    void updateSelectedColorOverlayInternal();

    void on_actionShow_Range_Buttons_toggled(bool arg1);

    void on_actionClusterColor_Preferences_triggered();

    void on_actionExport_Recolored_Images_triggered();

    void on_actionReset_Selected_Recolor_triggered();

    void on_actionScan_for_Orphaned_Colors_triggered();

    void on_actionClose_Palette_triggered();

    void on_actionClose_All_Images_triggered();

    void on_scanForOrphanButton_clicked();

    void on_actionShow_Missing_Color_Warning_toggled(bool arg1);

	void on_actionManual_Recolor_triggered();

private:
    Ui::ClusterColorMain *ui;

    bool updateOriginalDisplayEnable;
    bool updateControlDisplayEnable;
    bool updateRecolorDisplayEnable;
    bool updateSelectedColorOverlayEnable;

    void changeSelectedColors(std::shared_ptr<PaletteColor> newSelectedColor, bool add);
    void changeSelectedColors(const ColorList& newSelectedColors, bool add);

// Swatch editor methods
    // Reset all remap params on all swatches
    void clearRecolors();

    // Delete all group views in the palette
    void clearSwatches();

    // Clear and reconstruct all group views in the palette, maintaining selection
    void constructSwatches();

    // Push the colors from the selected group to the editor swatches
    void updateEditorSwatchColors();

    // Push all the values from the selected group to the editor sliders
    void updateEditorSliders();

    // Show the color picker for setting HSV shift
    void showPicker();

	void populateImageMenu();

    void linkScrolling(bool bypassVisibilityCheck = false);

    void updateUIEnabledState();

	// Local editor state
	ColorList selectedColors;
	QString selectedImage;
	std::shared_ptr<QImage> getSelectedImage();
	std::shared_ptr<PaletteGroup> selectedGroup;
	QActionGroup* selectedImageActionGroup;
	std::unique_ptr<Palette> backupPalette;
	QColorDialog* _colorPicker;
	std::vector<QMetaObject::Connection> scrollLinks;
	AutomaticGroupGeneration* _groupGen;
	PaletteGroupRemapParams _originalRemapParams; // store remap params temporarily while the color picker is open
    std::vector<PaletteGroupView*> _swatches;
};
#endif // CLUSTERCOLORMAIN_H
