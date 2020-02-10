#include "palettecolor.h"
#include <math.h>
#include <Comparison.h>

float PaletteColor::HueWeight = 1.0f;
float PaletteColor::SaturationWeight = 0.5f;
float PaletteColor::ValueWeight = 0.5f;
bool PaletteColor::UseCIEDistance = false;

// Create a color from integer RGB values
PaletteColor::PaletteColor(uint8_t r, uint8_t g, uint8_t b) : PaletteColor(QColor(r,g,b)) { }

// Create a color from a QColor
PaletteColor::PaletteColor(QColor color)
{
    _color = color;
    _remappedColor = color;
}

float PaletteColor::normalizeH(float h)
{
    while (h > 1.0f)
        h -= 1.0f;
    while (h < 0.0f)
        h += 1.0f;

    return h;
}

float PaletteColor::normalizeS(float s)
{
    return qBound(0.0f, s, 1.0f);
}

float PaletteColor::normalizeV(float v)
{
    return qBound(0.0f, v, 1.0f);
}

void PaletteColor::SetRemapParams(const QColor& originalColorCentroid, const PaletteGroupRemapParams& groupParams)
{
    if (groupParams.transparent)
    {
        _remappedColor = QColor(0,0,0,0);
    }
    else
    {
        // Get the offsets from the original centroid
        float dH = (_color.hueF() - originalColorCentroid.hueF());

        // normalize the angle
        while(dH < -0.5)
            dH += 1.0;
        while (dH > 0.5)
            dH -= 1.0;

        dH *= groupParams.hueFScale;
        float dS = (_color.saturationF() - originalColorCentroid.saturationF()) * groupParams.saturationFScale;
        float dV = (_color.valueF() - originalColorCentroid.valueF()) * groupParams.valueFScale;

        // Change the original color centroid into the remapped centroid, ignoring clipping
        float remapH = normalizeH(originalColorCentroid.hueF() + groupParams.hueFShift + dH);
        float remapS = normalizeS(originalColorCentroid.saturationF() + groupParams.saturationFShift + dS);
        float remapV = normalizeV(originalColorCentroid.valueF() + groupParams.valueFShift + dV);

        _remappedColor = QColor::fromHsvF(remapH, remapS, remapV).convertTo(QColor::Spec::Rgb);
    }
}

QColor PaletteColor::GetColor()
{
    return _color;
}

QColor PaletteColor::GetRemappedColor()
{
    return _remappedColor;
}

PaletteColor::KMeansVector PaletteColor::position()
{
    KMeansVector pos;

    if (UseCIEDistance)
    {
        ColorSpace::Hsv srcColor(_color.hueF()*360.0f, _color.saturationF() * SaturationWeight, _color.valueF() * ValueWeight);
        ColorSpace::Xyz dstColor;
        srcColor.To<ColorSpace::Xyz>(&dstColor);

        pos[0] = dstColor.x;
        pos[1] = dstColor.y;
        pos[2] = dstColor.z;
    }
    else
    {
        float angle = M_PI * 2.0f * _color.hueF();

        float r = 1.0;
        if (HueWeight >= SaturationWeight)
        {
            r = SaturationWeight * _color.saturationF() + (HueWeight-SaturationWeight);
        }
        else
        {
            r =  (SaturationWeight - HueWeight) * _color.saturationF();
        }

        pos[0] = r * cos(angle);
        pos[1] = r * sin(angle);
        pos[2] = _color.valueF() * ValueWeight;
    }

    return pos;
}

float PaletteColor::distance(const PaletteColor::KMeansVector& a, const PaletteColor::KMeansVector& b)
{
    if (UseCIEDistance)
    {
        ColorSpace::Xyz A(a[0], a[1], a[2]);
        ColorSpace::Xyz B(b[0], b[1], b[2]);

        return (float)ColorSpace::Cie1976Comparison::Compare(&A, &B);
    }
    else
    {
        return pow(a[0]-b[0],2) + pow(a[1]-b[1],2) + pow(a[2]-b[2],2);
    }
}

PaletteColor::KMeansVector PaletteColor::add(const PaletteColor::KMeansVector& a, const PaletteColor::KMeansVector& b)
{
    return KMeansVector{a[0] + b[0], a[1] + b[1], a[2] + b[2] };
}

void PaletteColor::accumulate(PaletteColor::KMeansVector& a, const PaletteColor::KMeansVector& b)
{
    a[0] += b[0];
    a[1] += b[1];
    a[2] += b[2];
}

PaletteColor::KMeansVector PaletteColor::scalarDivide(const PaletteColor::KMeansVector& a, float b)
{
    if (b == 0.0f)
        return a;

    return KMeansVector{a[0] / b, a[1] / b, a[2] / b };
}
