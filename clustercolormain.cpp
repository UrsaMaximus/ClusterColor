#include "clustercolormain.h"
#include "ui_clustercolormain.h"
#include "aboutdialog.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

ClusterColorMain::ClusterColorMain(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ClusterColorMain)
{
    setUnifiedTitleAndToolBarOnMac(true);
    ui->setupUi(this);

    _prefs = new Preferences(this);
    ui->actionShow_Missing_Color_Warning->setChecked(_prefs->GetSimpleBoolSetting("ShowOrphanWarning", true));

    selectedImageActionGroup = nullptr;
    palette = std::make_unique<Palette>();
    backupPalette = nullptr;

    ui->originalImageViewer->setTitle("Original");
    ui->controlImageViewer->setTitle("Group Key");
    ui->recolorImageViewer->setTitle("Recolor");

    // Link together all the image view scroll bars
    linkScrolling(true);

    // Connect up the click image to select color signals
    connect(ui->originalImageViewer, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));
    connect(ui->controlImageViewer, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));
    connect( ui->recolorImageViewer, SIGNAL(imageClicked(QPoint, ImageViewer*)), this, SLOT(imageClicked(QPoint, ImageViewer*)));

    // Connect up the wheel zoom signals
    connect(ui->originalImageViewer, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomed(QPoint, int)));
    connect(ui->controlImageViewer, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomed(QPoint, int)));
    connect( ui->recolorImageViewer, SIGNAL(imageWheelZoomed(QPoint, int)), this, SLOT(imageWheelZoomed(QPoint, int)));

    // Connect up the palette group remap signals
    connect(ui->selectedGroupShiftedSwatch, SIGNAL(swatchClicked(ClickableSwatch*)), this, SLOT(swatchClicked(ClickableSwatch*)));
    connect(ui->hueSlider, SIGNAL(onValueChanged(float)), this, SLOT(onHueSlider_valueChanged(float)));
    connect(ui->saturationSlider, SIGNAL(onValueChanged(float)), this, SLOT(onSaturationSlider_valueChanged(float)));
    connect(ui->valueSlider, SIGNAL(onValueChanged(float)), this, SLOT(onValueSlider_valueChanged(float)));

    // Configure the hue slider to be [-180 , 180º]
    ui->hueSlider->setValueUnit("º");
    ui->hueSlider->setRangeHalfWidth(180);

    connect(ui->selectedGroupShiftedSwatch, SIGNAL(swatchClicked(ClickableSwatch*)), this, SLOT(swatchClicked(ClickableSwatch*)));

    connect(ui->hueCompressionSlider, SIGNAL(onValueChanged(float)), this, SLOT(onHueCompressionSlider_valueChanged(float)));
    connect(ui->saturationCompressionSlider, SIGNAL(onValueChanged(float)), this, SLOT(onSaturationCompressionSlider_valueChanged(float)));
    connect(ui->valueCompressionSlider, SIGNAL(onValueChanged(float)), this, SLOT(onValueCompressionSlider_valueChanged(float)));

    // Create the color picker and connect its signals
    _colorPicker = new QColorDialog(this);
    _colorPicker->setModal(true);
    connect(_colorPicker, SIGNAL(colorSelected(const QColor&)), this, SLOT(on_newColorPreview(const QColor&)));
    connect(_colorPicker, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(on_newColorConfirmed(const QColor&)));
    connect(_colorPicker, SIGNAL(rejected()), this, SLOT(on_newColorRejected()));

    _groupGen = new AutomaticGroupGeneration(this);
    connect(_groupGen, SIGNAL(generateGroups(int, bool)), this, SLOT(autoGenerateGroups(int, bool)));
    connect(_groupGen, SIGNAL(accepted()), this, SLOT(deleteBackupPalette()));
    connect(_groupGen, SIGNAL(rejected()), this, SLOT(restoreBackupPalette()));

    updateUIEnabledState();

    updateOriginalDisplayEnable = true;
    updateControlDisplayEnable = true;
    updateRecolorDisplayEnable = true;
    updateSelectedColorOverlayEnable = true;
}

void ClusterColorMain::updateUIEnabledState()
{
    bool filesOpen = images.size() > 0;
    bool paletteOpen = palette->GetPaletteGroupCount() > 0;

    ui->actionZoom_In->setEnabled(filesOpen);
    ui->actionZoom_Out->setEnabled(filesOpen);
    ui->actionZoom_Fit->setEnabled(filesOpen);
    ui->actionExport_All->setEnabled(filesOpen && paletteOpen);
    ui->actionSelect_All->setEnabled(filesOpen);
    ui->actionSelect_None->setEnabled(filesOpen);
    ui->actionReset_Groups->setEnabled(paletteOpen);
    ui->actionReset_Recolors->setEnabled(paletteOpen);
    ui->actionSelect_Inverse->setEnabled(filesOpen);
    ui->actionExport_Index_Image->setEnabled(filesOpen && paletteOpen);
    ui->actionOpen_Original_Palette->setEnabled(true);
    ui->actionExport_Recolor_Palette->setEnabled(paletteOpen);
    ui->actionIsolate_Selected_Color->setEnabled(filesOpen);
    ui->actionExport_Original_Palette->setEnabled(paletteOpen);
    ui->actionOpen_Palette_as_Recolor->setEnabled(paletteOpen);
    ui->actionEdit_Selected_Color_Group->setEnabled(filesOpen && paletteOpen);
    ui->actionExport_Recolored_Images->setEnabled(filesOpen && paletteOpen);
    ui->actionAutomatic_Group_Generation->setEnabled(filesOpen);
    ui->actionAppend_Group_with_Selection->setEnabled(filesOpen && paletteOpen);
    ui->resetPalette->setEnabled(true);
    ui->singleGroupResetButton->setEnabled(paletteOpen);
    ui->actionReset_Selected_Recolor->setEnabled(paletteOpen);
    ui->actionScan_for_Orphaned_Colors->setEnabled(filesOpen);
    ui->scanForOrphanButton->setEnabled(filesOpen);
    ui->scanForOrphanButton->setVisible(!paletteOpen);
    ui->actionClose_All_Images->setEnabled(filesOpen);
    ui->actionClose_Palette->setEnabled(paletteOpen);
    ui->topControlsWidget->setVisible(filesOpen);
    ui->OpenImageToBeginBar->setVisible(!filesOpen);
}

ClusterColorMain::~ClusterColorMain()
{
    delete ui;
}

void ClusterColorMain::updateOriginalDisplayInternal()
{
    auto colorImage = getSelectedImage();
    if (colorImage != nullptr)
    {
        ui->originalImageViewer->SetImage(*colorImage);
    }
    else
    {
        ui->originalImageViewer->ClearImage();
    }
    updateOriginalDisplayEnable = true;
}

void ClusterColorMain::updateControlDisplayInternal()
{
    auto colorImage = getSelectedImage();
    if (colorImage != nullptr)
    {
        auto controlImage = palette->CreateControlImage(*colorImage);
        ui->controlImageViewer->SetImage(*controlImage);
    }
    else
    {
        ui->controlImageViewer->ClearImage();
    }
    updateControlDisplayEnable = true;
}

void ClusterColorMain::updateRecolorDisplayInternal()
{
    auto colorImage = getSelectedImage();
    if (colorImage != nullptr)
    {
        auto remapImage = palette->RecolorImage(*colorImage);
        ui->recolorImageViewer->SetImage(*remapImage);
    }
    else
    {
        ui->recolorImageViewer->ClearImage();
    }
    updateRecolorDisplayEnable = true;
}

void ClusterColorMain::updateSelectedColorOverlayInternal()
{
    auto colorImage = getSelectedImage();
    if (colorImage != nullptr && selectedColors.size() > 0)
    {
        auto selectionOverlay = palette->CreateSelectionOverlay(*colorImage, selectedColors);
        ui->originalImageViewer->SetSelectionOverlay(*selectionOverlay);
        ui->controlImageViewer->SetSelectionOverlay(*selectionOverlay);
        ui->recolorImageViewer->SetSelectionOverlay(*selectionOverlay);
    }
    else
    {
        ui->originalImageViewer->ClearSelectionOverlay();
        ui->controlImageViewer->ClearSelectionOverlay();
        ui->recolorImageViewer->ClearSelectionOverlay();
    }
    updateSelectedColorOverlayEnable = true;
}

void ClusterColorMain::updateOriginalDisplay()
{
    if (updateOriginalDisplayEnable)
    {
        updateOriginalDisplayEnable = false;
        QTimer::singleShot(16, this, SLOT(updateOriginalDisplayInternal()));
    }
}
void ClusterColorMain::updateControlDisplay()
{
    if (updateControlDisplayEnable)
    {
        updateControlDisplayEnable = false;
        QTimer::singleShot(16, this, SLOT(updateControlDisplayInternal()));
    }
}
void ClusterColorMain::updateRecolorDisplay()
{
    if (updateRecolorDisplayEnable)
    {
        updateRecolorDisplayEnable = false;
        QTimer::singleShot(16, this, SLOT(updateRecolorDisplayInternal()));
    }
}
void ClusterColorMain::updateSelectedColorOverlay()
{
    if (updateSelectedColorOverlayEnable)
    {
        updateSelectedColorOverlayEnable = false;
        QTimer::singleShot(0, this, SLOT(updateSelectedColorOverlayInternal()));
    }
}

void ClusterColorMain::clearRecolors()
{
    QMessageBox msgBox;
    msgBox.setText("Confirm Reset");
    msgBox.setInformativeText("Are you sure you want to reset all color remaps? Color groups will be preserved.");
    msgBox.setStandardButtons(QMessageBox::Reset | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Reset)
    {
        palette->ResetRemaps();
        constructSwatches();
        updateControlDisplay();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::imageWheelZoomed(QPoint viewportLocation, int delta)
{
    if (delta < 0)
    {
        ui->originalImageViewer->ZoomIn(viewportLocation);
        ui->controlImageViewer->ZoomIn(viewportLocation);
        ui->recolorImageViewer->ZoomIn(viewportLocation);
    }
    else if (delta > 0)
    {
        ui->originalImageViewer->ZoomOut(viewportLocation);
        ui->controlImageViewer->ZoomOut(viewportLocation);
        ui->recolorImageViewer->ZoomOut(viewportLocation);
    }
}

void ClusterColorMain::imageClicked(QPoint pixelLocation, ImageViewer* source)
{
    auto colorImage = getSelectedImage();
    if (colorImage != nullptr)
    {
        bool shiftHeld = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);

        if (source == ui->controlImageViewer)
        {
            auto group = palette->GetPaletteGroupContainingColor(colorImage->pixelColor(pixelLocation));

            if (group == nullptr && colorImage->pixelColor(pixelLocation).alphaF() > 0 && ui->actionShow_Missing_Color_Warning->isChecked())
            {
                QMessageBox msgBox;
                msgBox.setText("Missing color warning!");
                msgBox.setInformativeText("You clicked on a color not present in your palette. Colors not in your palette cannot be selected. Scan for orphan colors in the Color Groups menu to update you palette. This message can be disabled in the Selection menu.");
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
            }
            else if (group == nullptr)
            {
                changeSelectedColors(nullptr, shiftHeld);
            }
            else
            {
                changeSelectedColors(group->GetAllColors(), shiftHeld);
                changeSelectedGroup(group);
            }
        }
        else
        {
            auto paletteColor = palette->GetPaletteColor(colorImage->pixelColor(pixelLocation));

            if (paletteColor == nullptr && colorImage->pixelColor(pixelLocation).alphaF() > 0 && ui->actionShow_Missing_Color_Warning->isChecked())
            {
                QMessageBox msgBox;
                msgBox.setText("Missing color warning!");
                msgBox.setInformativeText("You clicked on a color not present in your palette. Colors not in your palette cannot be selected. Scan for orphan colors in the Color Groups menu to update you palette. This message can be disabled in the Selection menu.");
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
            }
            else
            {
                changeSelectedColors(paletteColor, shiftHeld);
            }
        }
    }
}

void ClusterColorMain::changeSelectedColors(const ColorList& newSelectedColors, bool add)
{
    if (!add)
        selectedColors.clear();

    for (auto& newSelectedColor : newSelectedColors)
    {
        if (newSelectedColor != nullptr && newSelectedColor->GetColor().alphaF() > 0)
        {
            if (std::find(selectedColors.begin(), selectedColors.end(), newSelectedColor) == selectedColors.end())
            {
                selectedColors.push_back(newSelectedColor);
            }
            else
            {
                selectedColors.erase(std::find(selectedColors.begin(), selectedColors.end(), newSelectedColor));
            }
        }
    }

    updateSelectedColorOverlay();
}

void ClusterColorMain::changeSelectedColors(std::shared_ptr<PaletteColor> newSelectedColor, bool add)
{
    if (!add)
        selectedColors.clear();

    if (newSelectedColor != nullptr && newSelectedColor->GetColor().alphaF() > 0)
    {
        if (std::find(selectedColors.begin(), selectedColors.end(), newSelectedColor) == selectedColors.end())
        {
            selectedColors.push_back(newSelectedColor);
        }
        else
        {
            selectedColors.erase(std::find(selectedColors.begin(), selectedColors.end(), newSelectedColor));
        }
    }

    updateSelectedColorOverlay();
}

void ClusterColorMain::clearSwatches()
{
    // Deselect all colors before clearing out the swatches
    changeSelectedColors(nullptr, false);

    // Clear out all the swatches in the swatch viewer
    for (auto* swatch : _swatches)
    {
        // Unhook this swatch from the palette group before deleting!
        swatch->SetPaletteGroup(nullptr);
    }

    for (auto* swatch : _swatches)
    {
        delete swatch;
    }

    _swatches.clear();
}

void ClusterColorMain::constructSwatches()
{
    // Should be a no-op if swatches were already cleared
    // and is required if they weren't
    clearSwatches();

    // Create the swatches
    for (int i=0; i < palette->GetPaletteGroupCount(); i++)
    {
        PaletteGroupView* swatch = new PaletteGroupView(palette->GetPaletteGroup(i));
        connect(swatch, SIGNAL(paletteGroupClicked(PaletteGroupView*)), this, SLOT(changeSelectedGroup(PaletteGroupView*)));
        ui->colorGroups->layout()->addWidget(swatch);
        _swatches.push_back(swatch);
    }

    // Reselect the selected swatch if needed
    for (auto* swatch : _swatches)
    {
        if (selectedGroup == swatch->GetPaletteGroup())
        {
            changeSelectedGroup(swatch);
        }
    }
}

std::shared_ptr<QImage> ClusterColorMain::getSelectedImage()
{
    if (images.find(selectedImage) == images.end())
        return nullptr;

    return images[selectedImage];
}

#define FileMenuActions {

void ClusterColorMain::on_actionOpen_Color_Image_triggered()
{
    bool sizeWarningDismissed = false;
    bool highColorWarningDismissed = false;
    bool allowChangePalette = palette->IsEmpty();  // Only allow automatic altering of empty palettes

    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open Color Images"), "", tr("Image Files (*.png)"));
    if (!filenames.isEmpty())
    {
        // Read in all the files and add their colors to the palette
        for (auto& filename : filenames)
        {
            auto img = std::make_unique<QImage>(filename);
            Palette backup = *palette;

            if (!sizeWarningDismissed && img->sizeInBytes() > 67108864l)
            {
                QMessageBox msgBox;
                msgBox.setText("Image Dimension Warning!");
                msgBox.setInformativeText("You are loading a very large image. The maximum reccomended image dimensions for ClusterColor are 4096x4096. If you open this image, you may experience very poor performance and crashes.");
                msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Cancel);
                int ret = msgBox.exec();

                if (ret == QMessageBox::Cancel)
                {
                    *palette = backup;
                    continue;
                }
                sizeWarningDismissed = true;
            }

            if (allowChangePalette)
            {
                palette->AddImageColors(*img);

                if (!highColorWarningDismissed && palette->GetPaletteColorCount() > 10000)
                {
                    QMessageBox msgBox;
                    msgBox.setText("Color Palette Warning!");
                    msgBox.setInformativeText("You have loaded an image containing over 10000 unique colors. ClusterColor works best on limited color sprite artwork. You may experience poor performance if you open this image.");
                    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Cancel);
                    int ret = msgBox.exec();

                    if (ret == QMessageBox::Cancel)
                    {
                        *palette = backup;
                        continue;
                    }
                    highColorWarningDismissed = true;
                }
            }

            images[filename] = std::move(img);
            selectedImage = filename;
        }

        populateImageMenu();

        // Generate the palette groups
        if (allowChangePalette)
        {
            palette->CreateOrphanGroup();
            constructSwatches();
        }

        // Update all displays
        updateOriginalDisplay();
        updateControlDisplay();
        updateRecolorDisplay();

        updateUIEnabledState();
    }
}

void ClusterColorMain::on_actionOpen_Index_Image_triggered()
{
    bool allowChangePalette = palette->IsEmpty();

    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open Index Images"), "", tr("Image Files (*.png)"));
    if (!filenames.isEmpty())
    {
        if (allowChangePalette)
        {
            // Try to autodetect the original palette
            QFileInfo firstImageInfo(filenames[0]);
            QDir selectedImageDir = firstImageInfo.dir();
            QString hash = firstImageInfo.fileName().right(12).left(8);
            QString paletteFileName = selectedImageDir.filePath(QString("Palette_p%1.png").arg(hash));
            if (!QFileInfo::exists(paletteFileName))
            {
                QMessageBox msgBox;
                msgBox.setText("The original palette for these index images could not be automatically found. Please select one.");
                msgBox.exec();

                paletteFileName = QFileDialog::getOpenFileName(this, tr("Locate Original Palette"), "", tr("Image Files (*.png)"));
            }

            if (!paletteFileName.isNull())
            {
                // Read in the palette
                try
                {
                    palette->LoadFromPaletteImage(QImage(paletteFileName), true, true, true);
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

        // Read in all the files and convert them to color images
        for (auto& filename : filenames)
        {
            images[filename] =  palette->CreateColorImage(QImage(filename));
            selectedImage = filename;
        }

        // Set a selected image and update all the displays
        populateImageMenu();

        constructSwatches();
        updateOriginalDisplay();
        updateControlDisplay();
        updateRecolorDisplay();

        updateUIEnabledState();
    }
}

void ClusterColorMain::on_actionOpen_Palette_as_Recolor_triggered()
{
    QString paletteFileName = QFileDialog::getOpenFileName(this, tr("Open Remap Palette"), "", tr("Image Files (*.png)"));

    if (!paletteFileName.isNull())
    {
        try
        {
            palette->LoadFromPaletteImage(QImage(paletteFileName), false, false, true);
        }
        catch (std::runtime_error& err)
        {
            QMessageBox msgBox;
            msgBox.setText(err.what());
            msgBox.exec();
            return;
        }

        constructSwatches();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::on_actionExport_Index_Image_triggered()
{
    QFileInfo selectedImageInfo(selectedImage);
    QDir selectedImageDir = selectedImageInfo.dir();

    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Save Index Images"), selectedImageDir.path());

    if (!dirPath.isNull())
    {
        try
        {
            QDir chosenDir(dirPath);
            for (auto& colorImage : images)
            {
                QFileInfo colorImagePath(colorImage.first);
                QString colorImageName = colorImagePath.fileName();
                colorImageName.truncate(colorImageName.size()-4);
                QString indexImageName = QString("%1_idx_p%2.png").arg(colorImageName, palette->GetPaletteStructureMD5(_prefs->PaletteExportIncludeMetadata()));
                QString indexImageFilePath = chosenDir.filePath(indexImageName);
                palette->CreateIndexImage(*colorImage.second, true)->save(indexImageFilePath);
            }

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

void ClusterColorMain::on_actionExport_Recolor_Palette_triggered()
{
    QString recolorPaletteName = QString("Recolor_p%1.png").arg(palette->GetPaletteStructureMD5(_prefs->PaletteExportIncludeMetadata()));
    QFileInfo selectedImageInfo(selectedImage);
    QDir selectedImageDir = selectedImageInfo.dir();
    QString recolorPath = selectedImageDir.filePath(recolorPaletteName);

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Recolor Palette"), recolorPath, tr("Image Files (*.png)"));

    if (!filename.isNull())
    {
        try
        {
            auto image = palette->SaveToPaletteImage(true,  _prefs->PaletteExportIncludeMetadata(), _prefs->PaletteExportAutosize(), _prefs->PaletteExportWidth(), _prefs->PaletteExportHeight());
            image->save(filename);
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

void ClusterColorMain::on_actionExport_Original_Palette_triggered()
{
    QString recolorPaletteName = QString("Palette_p%1.png").arg(palette->GetPaletteStructureMD5(_prefs->PaletteExportIncludeMetadata()));
    QFileInfo selectedImageInfo(selectedImage);
    QDir selectedImageDir = selectedImageInfo.dir();
    QString recolorPath = selectedImageDir.filePath(recolorPaletteName);

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Recolor Palette"), recolorPath, tr("Image Files (*.png)"));

    if (!filename.isNull())
    {
        try
        {
            auto image = palette->SaveToPaletteImage(false,  _prefs->PaletteExportIncludeMetadata(), _prefs->PaletteExportAutosize(), _prefs->PaletteExportWidth(), _prefs->PaletteExportHeight());
            image->save(filename);
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

void ClusterColorMain::on_actionExport_All_triggered()
{
    QFileInfo selectedImageInfo(selectedImage);
    QDir selectedImageDir = selectedImageInfo.dir();

    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Save Recolor Set"), selectedImageDir.path());

    if (!dirPath.isNull())
    {
        try
        {

            QDir chosenDir(dirPath);
            for (auto& colorImage : images)
            {
                QFileInfo colorImagePath(colorImage.first);
                QString colorImageName = colorImagePath.fileName();
                colorImageName.truncate(colorImageName.size()-4);
                QString indexImageName = QString("%1_indexed_p%2.png").arg(colorImageName, palette->GetPaletteStructureMD5(_prefs->PaletteExportIncludeMetadata()));
                QString indexImageFilePath = chosenDir.filePath(indexImageName);
                palette->CreateIndexImage(*colorImage.second, _prefs->PaletteExportIncludeMetadata())->save(indexImageFilePath);
            }

            QString originalName = QString("Palette_p%1.png").arg(palette->GetPaletteStructureMD5(_prefs->PaletteExportIncludeMetadata()));
            QString originalPath = chosenDir.filePath(originalName);
            auto originalImage = palette->SaveToPaletteImage(false, _prefs->PaletteExportIncludeMetadata(), _prefs->PaletteExportAutosize(), _prefs->PaletteExportWidth(), _prefs->PaletteExportHeight());
            originalImage->save(originalPath);

            QString recolorName = QString("Recolor_p%1.png").arg(palette->GetPaletteStructureMD5(_prefs->PaletteExportIncludeMetadata()));
            QString recolorPath = chosenDir.filePath(recolorName);
            auto recolorImage = palette->SaveToPaletteImage(true,  _prefs->PaletteExportIncludeMetadata(), _prefs->PaletteExportAutosize(), _prefs->PaletteExportWidth(), _prefs->PaletteExportHeight());
            recolorImage->save(recolorPath);

            QMessageBox msgBox;
            msgBox.setText("Indexed versions of all open images, the original palette, and the recolor palette have all been saved to the selected directory.");
            msgBox.exec();

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

void ClusterColorMain::on_actionExport_Recolored_Images_triggered()
{
    QFileInfo selectedImageInfo(selectedImage);
    QDir selectedImageDir = selectedImageInfo.dir();

    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Save Recolored Images"), selectedImageDir.path());

    if (!dirPath.isNull())
    {
        try
        {
            QDir chosenDir(dirPath);
            for (auto& colorImage : images)
            {
                QFileInfo colorImagePath(colorImage.first);
                QString colorImageName = colorImagePath.fileName();
                colorImageName.truncate(colorImageName.size()-4);
                QString indexImageName = QString("%1_Recolored.png").arg(colorImageName);
                QString indexImageFilePath = chosenDir.filePath(indexImageName);
                palette->RecolorImage(*colorImage.second)->save(indexImageFilePath);
            }
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

void ClusterColorMain::on_actionOpen_Original_Palette_triggered()
{
    QString paletteFileName = QFileDialog::getOpenFileName(this, tr("Open Original Palette"), "", tr("Image Files (*.png)"));

    if (!paletteFileName.isNull())
    {
        // Clear the palette
        *palette = Palette();
        clearSwatches();

        try
        {
            palette->LoadFromPaletteImage(QImage(paletteFileName), true, true, true);
        }
        catch (std::runtime_error& err)
        {
            QMessageBox msgBox;
            msgBox.setText(err.what());
            msgBox.exec();
            return;
        }

        constructSwatches();
        updateControlDisplay();
        updateRecolorDisplay();
        updateUIEnabledState();
    }
}

#define EndFileMenuActions }

#define ViewMenuActions {

void ClusterColorMain::on_actionZoom_In_triggered()
{
    ui->originalImageViewer->ZoomIn();
    ui->controlImageViewer->ZoomIn();
    ui->recolorImageViewer->ZoomIn();
}

void ClusterColorMain::on_actionZoom_Out_triggered()
{
    ui->originalImageViewer->ZoomOut();
    ui->controlImageViewer->ZoomOut();
    ui->recolorImageViewer->ZoomOut();
}

void ClusterColorMain::on_actionZoom_Fit_triggered()
{
    ui->originalImageViewer->Zoom1x();
    ui->controlImageViewer->Zoom1x();
    ui->recolorImageViewer->Zoom1x();
}

void ClusterColorMain::on_actionShow_Original_Color_toggled(bool arg1)
{
    ui->originalImageViewer->setVisible(arg1);
    linkScrolling();

    // If this display is being shown again, have one of the other displays emit a scroll sync event
    if (arg1 && ui->controlImageViewer->isVisible())
    {
        ui->controlImageViewer->EmitScrollEvent();
    }
    else if (arg1 && ui->recolorImageViewer->isVisible())
    {
        ui->recolorImageViewer->EmitScrollEvent();
    }
}

void ClusterColorMain::on_actionShow_Control_Map_toggled(bool arg1)
{
    ui->controlImageViewer->setVisible(arg1);
    linkScrolling();

    // If this display is being shown again, have one of the other displays emit a scroll sync event
    if (arg1 && ui->originalImageViewer->isVisible())
    {
        ui->originalImageViewer->EmitScrollEvent();
    }
    else if (arg1 && ui->recolorImageViewer->isVisible())
    {
        ui->recolorImageViewer->EmitScrollEvent();
    }
}

void ClusterColorMain::on_actionShow_Remapped_Color_toggled(bool arg1)
{
    ui->recolorImageViewer->setVisible(arg1);
    linkScrolling();

    // If this display is being shown again, have one of the other displays emit a scroll sync event
    if (arg1 && ui->originalImageViewer->isVisible())
    {
        ui->originalImageViewer->EmitScrollEvent();
    }
    else if (arg1 && ui->controlImageViewer->isVisible())
    {
        ui->controlImageViewer->EmitScrollEvent();
    }
}

void ClusterColorMain::linkScrolling(bool bypassVisibilityCheck)
{
    // unlink all
    for (auto& link : scrollLinks)
    {
        QObject::disconnect(link);
    }
    scrollLinks.clear();

    if (bypassVisibilityCheck || (ui->originalImageViewer->isVisible() && ui->controlImageViewer->isVisible()))
    {
        scrollLinks.push_back(connect(ui->originalImageViewer, SIGNAL(scrollChanged(int,int)), ui->controlImageViewer, SLOT(onScrollChanged(int,int))));
        scrollLinks.push_back(connect(ui->controlImageViewer, SIGNAL(scrollChanged(int,int)), ui->originalImageViewer, SLOT(onScrollChanged(int,int))));
    }

    if (bypassVisibilityCheck || (ui->originalImageViewer->isVisible() && ui->recolorImageViewer->isVisible()))
    {
        scrollLinks.push_back(connect(ui->originalImageViewer, SIGNAL(scrollChanged(int,int)), ui->recolorImageViewer, SLOT(onScrollChanged(int,int))));
        scrollLinks.push_back(connect(ui->recolorImageViewer, SIGNAL(scrollChanged(int,int)), ui->originalImageViewer, SLOT(onScrollChanged(int,int))));
    }

    if (bypassVisibilityCheck || (ui->controlImageViewer->isVisible() && ui->recolorImageViewer->isVisible()))
    {
        scrollLinks.push_back(connect(ui->controlImageViewer, SIGNAL(scrollChanged(int,int)), ui->recolorImageViewer, SLOT(onScrollChanged(int,int))));
        scrollLinks.push_back(connect(ui->recolorImageViewer, SIGNAL(scrollChanged(int,int)), ui->controlImageViewer, SLOT(onScrollChanged(int,int))));
    }
}

#define EndViewMenuActions }

#define SelectMenuActions {

void ClusterColorMain::on_actionAppend_Group_with_Selection_triggered()
{
    if (selectedColors.size() > 0 && selectedGroup != nullptr)
    {
        palette->AppendGroup(selectedColors, selectedGroup);
        changeSelectedColors(nullptr, false);
        constructSwatches();
        updateRecolorDisplay();
        updateControlDisplay();
    }
}

void ClusterColorMain::on_actionSelect_All_triggered()
{
    changeSelectedColors(palette->GetAllColors(), false);
}

void ClusterColorMain::on_actionSelect_Inverse_triggered()
{
    ColorList allColors = palette->GetAllColors();
    for (auto& color : selectedColors)
    {
        allColors.erase(std::find(allColors.begin(), allColors.end(), color));
    }

    changeSelectedColors(allColors, false);
}

void ClusterColorMain::on_actionIsolate_Selected_Color_triggered()
{
    if (selectedColors.size() > 0)
    {
        auto newGroup = palette->CreateGroup(selectedColors);
        selectedGroup = newGroup;
        constructSwatches();
        updateRecolorDisplay();
        updateControlDisplay();
    }
}

void ClusterColorMain::on_actionSelect_None_triggered()
{
    changeSelectedColors(nullptr, false);
}

void ClusterColorMain::on_actionEdit_Selected_Color_Group_triggered()
{
    if (selectedColors.size() > 0)
    {
        for (auto& swatch : _swatches)
        {
            if (swatch->GetPaletteGroup()->ContainsColor(selectedColors[0]->GetColor()))
            {
                // Deselect the color so as not to visually spoil the recolor process
                // then select the relevant group in the editor.
                changeSelectedColors(nullptr, false);
                changeSelectedGroup(swatch);
                return;
            }
        }
    }
}

#define EndSelectMenuActions }

#define ColorGroupsMenuActions {

void ClusterColorMain::autoGenerateGroups(int numberOfClusters, bool saveRecolors)
{
    // Remove all the swatch user interface elements
    clearSwatches();

    // Save the recolor values from the palette before recreating groups
    std::vector<PaletteGroupRemapParams> savedRecolors;
    for (int i=0; i < palette->GetPaletteGroupCount(); i++)
    {
        savedRecolors.push_back(palette->GetPaletteGroup(i)->GetRemapParams());
    }

    // Redo the group creation (twice because my k-means implementation is broken)
    palette->CreateGroupsByClustering(numberOfClusters);
    // palette.CreateGroupsByClustering(numberOfClusters); // Still needed?

    // Restore the recolors we can
    if (saveRecolors)
    {
        for (int i=0; i < palette->GetPaletteGroupCount(); i++)
        {
            if ((int)savedRecolors.size() > i)
                palette->GetPaletteGroup(i)->SetRemapParams(savedRecolors[i]);
            else
                break;
        }
    }

    // Reconstruct the swatch UI elements
    constructSwatches();

    // Update the displays
    updateControlDisplay();
    updateRecolorDisplay();

    updateUIEnabledState();
}

void ClusterColorMain::on_actionAutomatic_Group_Generation_triggered()
{
    // Create a copy of the current palette as backup
    createBackupPalette();

    _groupGen->exec();
}

void ClusterColorMain::createBackupPalette()
{
    backupPalette = std::make_unique<Palette>(*palette);
}

void ClusterColorMain::deleteBackupPalette()
{
    backupPalette = nullptr;
}

void ClusterColorMain::restoreBackupPalette()
{
    if (backupPalette != nullptr)
    {
        palette = std::move(backupPalette);
        backupPalette = nullptr;
        constructSwatches();
        updateControlDisplay();
        updateRecolorDisplay();
        updateUIEnabledState();
    }
}

void ClusterColorMain::on_actionReset_Recolors_triggered()
{
    clearRecolors();
}

void ClusterColorMain::on_actionReset_Groups_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("Confirm Reset");
    msgBox.setInformativeText("Are you sure you want to reset all groups? This will delete all created groups and erase all color remaps.");
    msgBox.setStandardButtons(QMessageBox::Reset | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Reset)
    {
        palette->Reset();
        on_actionScan_for_Orphaned_Colors_triggered();
    }
    updateUIEnabledState();
}

#define EndColorGroupsMenuActions }

#define ImageMenuActions {

void ClusterColorMain::populateImageMenu()
{
    ui->menuImages->clear();
    if (selectedImageActionGroup != nullptr)
    {
        delete selectedImageActionGroup;
        selectedImageActionGroup = nullptr;
    }

    selectedImageActionGroup = new QActionGroup(this);

    selectedImageActionGroup->setExclusive(true);

    connect(selectedImageActionGroup, SIGNAL(triggered(QAction *)), this, SLOT(on_changedSelectedFile(QAction *)));

    for (auto& imgPair : images)
    {
        QAction* fileSelectAction = new QAction(imgPair.first, selectedImageActionGroup);
        fileSelectAction->setCheckable(true);
        ui->menuImages->addAction(fileSelectAction);
    }
}

void ClusterColorMain::on_changedSelectedFile(QAction * action)
{
    selectedImage = action->text();
    updateOriginalDisplay();
    updateControlDisplay();
    updateRecolorDisplay();
    updateSelectedColorOverlay();
}

#define EndImageMenuActions }

#define ColorGroupEditor {

void ClusterColorMain::changeSelectedGroup(std::shared_ptr<PaletteGroup> selected)
{
    for (auto& swatch : _swatches)
    {
        if (swatch->GetPaletteGroup() == selected)
        {
            changeSelectedGroup(swatch);
            break;
        }
    }
}

void ClusterColorMain::changeSelectedGroup(PaletteGroupView* selected)
{
    if (selected != nullptr)
    {
        // Set the selected group and select the group view
        selectedGroup = selected->GetPaletteGroup();
        ui->RecolorControlGroup->setEnabled(true);
    }
    else
    {
        selectedGroup = nullptr;
        ui->RecolorControlGroup->setEnabled(false);
    }

    // Update palette selection in the GUI
    for (auto& groupView : _swatches)
    {
        groupView->setSelected(groupView == selected);
    }

    // Update the swatch colors and sliders in the editor pane
    updateEditorSwatchColors();
    updateEditorSliders();
}

void ClusterColorMain::updateEditorSliders()
{
    if (selectedGroup != nullptr)
    {
        ui->hueSlider->setValue(selectedGroup->GetRemapParams().hueFShift * 360.0f);
        ui->saturationSlider->setValue(selectedGroup->GetRemapParams().saturationFShift * 100.0f);
        ui->valueSlider->setValue(selectedGroup->GetRemapParams().valueFShift * 100.0f);

        ui->hueCompressionSlider->setValue(selectedGroup->GetRemapParams().hueFScale);
        ui->saturationCompressionSlider->setValue(selectedGroup->GetRemapParams().saturationFScale);
        ui->valueCompressionSlider->setValue(selectedGroup->GetRemapParams().valueFScale);

        ui->opaqueCheckbox->setChecked(!selectedGroup->GetRemapParams().transparent);
    }
}

void ClusterColorMain::updateEditorSwatchColors()
{
    if (selectedGroup != nullptr)
    {
        // set backgrounds
        //QPalette pal = QMainWindow::palette();
        //pal.setColor(QPalette::Background, selectedGroup->GetOriginalColorCentroid());
        //ui->selectedGroupControlSwatch->setPalette(pal);
        //ui->selectedGroupControlSwatch->setAutoFillBackground(true);

        QPalette palRecolor = QMainWindow::palette();
        palRecolor.setColor(QPalette::Background, selectedGroup->GetRemapColorCentroid());
        ui->selectedGroupShiftedSwatch->setPalette(palRecolor);
        ui->selectedGroupShiftedSwatch->setAutoFillBackground(true);

        // Also update the palette swatch in the main gui
        for (auto& swatch : _swatches)
        {
            if (swatch->GetPaletteGroup() == selectedGroup)
            {
                // Set the selected group to force a visual update
                swatch->SetPaletteGroup(selectedGroup);
            }
        }
    }
    else
    {
        //QPalette pal = QMainWindow::palette();
        //ui->selectedGroupControlSwatch->setPalette(pal);
        //ui->selectedGroupControlSwatch->setAutoFillBackground(true);

        QPalette palRecolor = QMainWindow::palette();
        ui->selectedGroupShiftedSwatch->setPalette(palRecolor);
        ui->selectedGroupShiftedSwatch->setAutoFillBackground(true);
    }
}

void ClusterColorMain::showPicker()
{
    if (selectedGroup != nullptr)
    {
        _originalRemapParams = selectedGroup->GetRemapParams();
        _colorPicker->setCurrentColor(selectedGroup->GetRemapColorCentroid());
        _colorPicker->show();
     }
}

void ClusterColorMain::swatchClicked(ClickableSwatch* swatch)
{
    //ignore(swatch);
    showPicker();
}

void ClusterColorMain::on_newColorRejected()
{
    if (selectedGroup != nullptr)
    {
        selectedGroup->SetRemapParams(_originalRemapParams); // No need to update because set calls update
        updateEditorSliders();
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::on_newColorPreview(const QColor& color)
{
    if (selectedGroup != nullptr)
    {
        selectedGroup->SetRemapParams(color); // No need to update because set calls update
        updateEditorSliders();
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::on_newColorConfirmed(const QColor& color)
{
    if (selectedGroup != nullptr)
    {
        selectedGroup->SetRemapParams(color); // No need to update because set calls update
        updateEditorSliders();
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::onHueSlider_valueChanged(float value)
{
    if (selectedGroup != nullptr)
    {
        // Push changes to the selected group
        selectedGroup->GetRemapParams().hueFShift = value / 360.0f;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::onSaturationSlider_valueChanged(float value)
{
    if (selectedGroup != nullptr)
    {
        // Push changes to the selected group
        selectedGroup->GetRemapParams().saturationFShift = value / 100.0f;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::onValueSlider_valueChanged(float value)
{
    if (selectedGroup != nullptr)
    {
        // Push changes to the selected group
        selectedGroup->GetRemapParams().valueFShift = value / 100.0f;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();

    }
}

void ClusterColorMain::onHueCompressionSlider_valueChanged(float value)
{
    if (selectedGroup != nullptr)
    {
        // Push changes to the selected group
        selectedGroup->GetRemapParams().hueFScale = value;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::onSaturationCompressionSlider_valueChanged(float value)
{
    if (selectedGroup != nullptr)
    {
        // Push changes to the selected group
        selectedGroup->GetRemapParams().saturationFScale = value;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::onValueCompressionSlider_valueChanged(float value)
{
    if (selectedGroup != nullptr)
    {
        // Push changes to the selected group
        selectedGroup->GetRemapParams().valueFScale = value;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();

    }
}

void ClusterColorMain::on_singleGroupResetButton_clicked()
{
    if (selectedGroup != nullptr)
    {
        selectedGroup->SetRemapParams(PaletteGroupRemapParams()); // No need to update because set calls update
        updateEditorSliders();
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::on_opaqueCheckbox_toggled(bool checked)
{
    if (selectedGroup != nullptr)
    {
        selectedGroup->GetRemapParams().transparent = !checked;
        selectedGroup->UpdateRemaps(); // Call this after changing any remap values
        updateEditorSwatchColors();
        updateRecolorDisplay();
    }
}

void ClusterColorMain::on_resetPalette_clicked()
{
    clearRecolors();
}

#define EndPaletteGroupEdit }

void ClusterColorMain::on_actionAbout_ClusterColor_triggered()
{
    AboutDialog* ad = new AboutDialog(this);
    ad->exec();
    ad->deleteLater();
}

void ClusterColorMain::on_actionShow_Range_Buttons_toggled(bool arg1)
{
    ui->saturationSlider->setShowRangeButtons(arg1);
    ui->valueSlider->setShowRangeButtons(arg1);
}

void ClusterColorMain::on_actionClusterColor_Preferences_triggered()
{
    _prefs->Revert();
    _prefs->exec();
}

void ClusterColorMain::on_actionReset_Selected_Recolor_triggered()
{
    on_singleGroupResetButton_clicked();
}

void ClusterColorMain::on_actionScan_for_Orphaned_Colors_triggered()
{
    int origSwatchSize = _swatches.size();

    // Go over all open images and add their colors
    for (auto& imgPair : images)
    {
        palette->AddImageColors(*imgPair.second);
    }

    // Tell the palette to create an orphan group
    palette->CreateOrphanGroup();

    // Update the swatches
    constructSwatches();

    // Select the last swatch if a group was created
    if (_swatches.size() > (int)origSwatchSize)
        changeSelectedGroup(_swatches[_swatches.size()-1]);

    updateControlDisplay();
    updateRecolorDisplay();
    updateUIEnabledState();
}

void ClusterColorMain::on_actionClose_Palette_triggered()
{
    palette->Reset();
    clearSwatches();
    changeSelectedGroup(nullptr);
    updateUIEnabledState();
    updateOriginalDisplay();
    updateControlDisplay();
    updateRecolorDisplay();
}

void ClusterColorMain::on_actionClose_All_Images_triggered()
{
    selectedImage = nullptr;
    images.clear();
    populateImageMenu();
    updateUIEnabledState();
    updateOriginalDisplay();
    updateControlDisplay();
    updateRecolorDisplay();
}

void ClusterColorMain::on_scanForOrphanButton_clicked()
{
    on_actionScan_for_Orphaned_Colors_triggered();
    updateUIEnabledState();
}

void ClusterColorMain::on_actionShow_Missing_Color_Warning_toggled(bool arg1)
{
    _prefs->SetSimpleBoolSetting("ShowOrphanWarning", arg1);
}
