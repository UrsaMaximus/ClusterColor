#ifndef PALETTEGROUPVIEW_H
#define PALETTEGROUPVIEW_H

#include <QWidget>
#include <QColorDialog>
#include <palettegroup.h>
#include <clickableswatch.h>

namespace Ui {
class PaletteGroupView;
}

class PaletteGroupView : public QWidget
{
    Q_OBJECT

public:
    explicit PaletteGroupView(std::shared_ptr<PaletteGroup> paletteGroup, QWidget *parent = nullptr);
    ~PaletteGroupView();

    void SetPaletteGroup(std::shared_ptr<PaletteGroup>);
    std::shared_ptr<PaletteGroup> GetPaletteGroup();

    bool selected();
    void setSelected(bool selected);

signals:
    // Fires if any part of the palette is clicked
    void paletteGroupClicked(PaletteGroupView*);

    // Fires only if the control color swatch was clicked
    void paletteGroupControlClicked(PaletteGroupView*);

    // Fires only if the remapped color swatch was clicked
    void paletteGroupShiftedClicked(PaletteGroupView*);

private slots:
    void swatchClicked(ClickableSwatch*);

private:
    Ui::PaletteGroupView *ui;
    std::shared_ptr<PaletteGroup> _paletteGroup;
    bool _selected;

    void updateSwatchColors();
};

#endif // PALETTEGROUPVIEW_H
