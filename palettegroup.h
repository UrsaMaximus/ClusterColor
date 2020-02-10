#ifndef PALETTEGROUP_H
#define PALETTEGROUP_H
#include <map>
#include "palettecolor.h"

class PaletteGroup
{
public:
    PaletteGroup(ColorList colors);

    // Set all remap params with the provided structure
    void SetRemapParams(PaletteGroupRemapParams remapParameters);

    // Derive remap params from the provided remap color.
    // Scale changes are preserved
    void SetRemapParams(QColor remapColor);

    PaletteGroupRemapParams& GetRemapParams();
    void UpdateRemaps();

    // Get the centroid color of the group. This color is safe to use in calculations, as
    // is is guaranteed to exist in clipped HSV space.
    QColor GetOriginalColorCentroid();

    // Get the centroid remap color. This is display-only. Don't use it to calculate anything
    // as the true value might be an impossible color.
    QColor GetRemapColorCentroid();

    void AddColor(std::shared_ptr<PaletteColor> color);
    void RemoveColor(std::shared_ptr<PaletteColor> color);

    bool ContainsColor(const QColor& color);

    ColorList GetAllColors();

    int GetColorCount();

private:
    // Iterate over all colors and get the original centroid, then calculate the remap centroid
    void calculateCentroids();

    // Update remaps of all subcolors.
    void updateRemaps();

    ColorList _colors;
    std::map<QRgb, std::shared_ptr<PaletteColor>> _colorMap;
    PaletteGroupRemapParams _remapParams;
    QColor _originalCentroid;
    QColor _remapCentroid;
};

typedef std::vector<std::shared_ptr<PaletteGroup>> GroupList;

#endif // PALETTEGROUP_H
