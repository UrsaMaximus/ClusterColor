#include "palettegroup.h"
#include <math.h>

PaletteGroup::PaletteGroup(ColorList colors)
{
    _colors = colors;
    _colorMap = ColorMapFromColorList(_colors);

    calculateCentroids();
    updateRemaps();
}

// Set all remap params with the provided structure
void PaletteGroup::SetRemapParams(PaletteGroupRemapParams remapParameters)
{
    // Just push these params right in
    _remapParams = remapParameters;

    calculateCentroids();
    updateRemaps();
}

// Derive remap params from the provided remap color.
// Scale changes are preserved
void PaletteGroup::SetRemapParams(QColor remapColor)
{
    // Get the offsets from the original centroid
    _remapParams.hueFShift = (remapColor.hueF() - _originalCentroid.hueF());

    // Normalize the hue shift
    while(_remapParams.hueFShift < -0.5)
        _remapParams.hueFShift += 1.0;
    while (_remapParams.hueFShift > 0.5)
        _remapParams.hueFShift -= 1.0;

    _remapParams.saturationFShift = (remapColor.saturationF() - _originalCentroid.saturationF());
    _remapParams.valueFShift = (remapColor.valueF() - _originalCentroid.valueF());

    calculateCentroids();
    updateRemaps();
}

PaletteGroupRemapParams& PaletteGroup::GetRemapParams()
{
    return _remapParams;
}

void PaletteGroup::UpdateRemaps()
{
    calculateCentroids();
    updateRemaps();
}

// Get the centroid color of the group
QColor PaletteGroup::GetOriginalColorCentroid()
{
    return _originalCentroid;
}

// Get the centroid remap color. This is display-only. Don't use it to calculate anything
// as it might be clipped.
QColor PaletteGroup::GetRemapColorCentroid()
{
    return _remapCentroid;
}

// Iterate over all colors and get the original centroid, then calculate the remap centroid
void PaletteGroup::calculateCentroids()
{
    double hX = 0;
    double hY = 0;
    double s = 0;
    double v = 0;
    for (auto& color : _colors)
    {
        hX += cos(color->GetColor().hueF() * 2 * M_PI);
        hY += sin(color->GetColor().hueF() * 2 * M_PI);
        s += color->GetColor().saturationF();
        v += color->GetColor().valueF();
    }

    _originalCentroid = QColor::fromHsvF(PaletteColor::normalizeH(atan2(hY, hX) / (2 * M_PI)),
                                         PaletteColor::normalizeS(s / (double) _colors.size()),
                                         PaletteColor::normalizeV(v / (double) _colors.size())).convertTo(QColor::Spec::Rgb);

    _remapCentroid = QColor::fromHsvF(PaletteColor::normalizeH(_originalCentroid.hueF() + _remapParams.hueFShift),
                                      PaletteColor::normalizeS(_originalCentroid.saturationF() + _remapParams.saturationFShift),
                                      PaletteColor::normalizeV(_originalCentroid.valueF() + _remapParams.valueFShift)).convertTo(QColor::Spec::Rgb);

}

// Update remaps of all subcolors.
void PaletteGroup::updateRemaps()
{
    for (auto& color : _colors)
    {
        color->SetRemapParams(_originalCentroid, _remapParams);
    }
}

int PaletteGroup::GetColorCount()
{
    return _colors.size();
}

void PaletteGroup::AddColor(std::shared_ptr<PaletteColor> color)
{
    if (!ContainsColor(color->GetColor()))
    {
        _colors.push_back(color);
        _colorMap[color->GetColor().rgba()] = color;

        // Adding a color from the group invalidates the centroids and the color calculations of all member colors
        calculateCentroids();
        updateRemaps();
    }
}

void PaletteGroup::RemoveColor(std::shared_ptr<PaletteColor> color)
{
    for (auto itr = _colors.begin(); itr != _colors.end(); ++itr)
    {
        if (*itr == color)
        {
            if (_colors.size() == 1)
                throw std::runtime_error("Cannot remove the last color from a group!");
            _colorMap.erase(color->GetColor().rgba());
            _colors.erase(itr);
            break;
        }
    }

    // Removing a color from the group invalidates the centroids and the color calculations of all member colors
    calculateCentroids();
    updateRemaps();
}

ColorList PaletteGroup::GetAllColors()
{
    return _colors;
}

bool PaletteGroup::ContainsColor(const QColor& color)
{
    return _colorMap.find(color.rgba()) != _colorMap.end();
}
