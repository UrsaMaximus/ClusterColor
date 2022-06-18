#include "palettegroupview.h"
#include "ui_palettegroupview.h"

PaletteGroupView::PaletteGroupView(std::shared_ptr<PaletteGroup> paletteGroup, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PaletteGroupView)
{
    _paletteGroup = paletteGroup;
    ui->setupUi(this);

    _selected = false;

    connect(ui->recolorSwatch, SIGNAL(swatchClicked(ClickableSwatch*)), this, SLOT(swatchClicked(ClickableSwatch*)));
    connect(ui->colorSwatch, SIGNAL(swatchClicked(ClickableSwatch*)), this, SLOT(swatchClicked(ClickableSwatch*)));

    // Setup some controls
    updateSwatchColors();
    setSelected(false);
}

PaletteGroupView::~PaletteGroupView()
{
    _paletteGroup = nullptr;
    delete ui;
}

bool PaletteGroupView::selected()
{
    return _selected;
}

void PaletteGroupView::setSelected(bool selected)
{
    _selected = selected;

    if (_selected)
        setStyleSheet("#frame { border: 2px solid yellow; }");
    else
        setStyleSheet("#frame { border: 2px solid black; }");

    updateSwatchColors();
}

void PaletteGroupView::SetPaletteGroup(std::shared_ptr<PaletteGroup> group)
{
    _paletteGroup = group;
    updateSwatchColors();
}

std::shared_ptr<PaletteGroup> PaletteGroupView::GetPaletteGroup()
{
    return _paletteGroup;
}

void PaletteGroupView::updateSwatchColors()
{
    if (_paletteGroup != nullptr)
    {
        // set backgrounds
        QPalette pal = palette();
		pal.setColor(QPalette::Window, _paletteGroup->GetOriginalColorCentroid());
        ui->colorSwatch->setPalette(pal);
        ui->colorSwatch->setAutoFillBackground(true);

        QPalette palRecolor = palette();
		palRecolor.setColor(QPalette::Window, _paletteGroup->GetRemapColorCentroid());
        ui->recolorSwatch->setPalette(palRecolor);
        ui->recolorSwatch->setAutoFillBackground(true);
    }
}

void PaletteGroupView::swatchClicked(ClickableSwatch* swatch)
{
    if (swatch == ui->colorSwatch)
    {
        paletteGroupControlClicked(this);
    }
    if (swatch == ui->recolorSwatch)
    {
        paletteGroupShiftedClicked(this);
    }

    paletteGroupClicked(this);
}
