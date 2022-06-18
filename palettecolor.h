#ifndef PALETTECOLOR_H
#define PALETTECOLOR_H

#include <QColor>
#include <memory>
#include <vector>
#include <array>
#include <map>

struct PaletteGroupRemapParams
{
    float hueFShift;
    float saturationFShift;
    float valueFShift;

    float hueFScale;
    float saturationFScale;
    float valueFScale;

    bool transparent;


    PaletteGroupRemapParams()
    {
        hueFShift = 0;
        saturationFShift = 0;
        valueFShift = 0;

        hueFScale = 1;
        saturationFScale = 1;
        valueFScale = 1;

        transparent = false;
    }

    static QColor colorFromFloat(float f)
    {
        uint8_t* floatData = (uint8_t*)&f;
        return QColor(floatData[0],floatData[1],floatData[2],floatData[3]);
    }

    static float floatFromColor(const QColor& c)
    {
        float f;
        uint8_t* floatData = (uint8_t*)&f;
        floatData[0] = (uint8_t)c.red();
        floatData[1] = (uint8_t)c.green();
        floatData[2] = (uint8_t)c.blue();
        floatData[3] = (uint8_t)c.alpha();

        return f;
    }

    PaletteGroupRemapParams(std::vector<QColor> rgbRep)
    {
        hueFShift = floatFromColor(rgbRep[0]);
        saturationFShift = floatFromColor(rgbRep[1]);
        valueFShift = floatFromColor(rgbRep[2]);

        hueFScale = floatFromColor(rgbRep[3]);
        saturationFScale = floatFromColor(rgbRep[4]);
        valueFScale = floatFromColor(rgbRep[5]);
    }

    std::vector<QColor> createRgbRep()
    {
        std::vector<QColor> rgbRep(6);
        rgbRep[0] = colorFromFloat(hueFShift);
        rgbRep[1] = colorFromFloat(saturationFShift);
        rgbRep[2] = colorFromFloat(valueFShift);

        rgbRep[3] = colorFromFloat(hueFScale);
        rgbRep[4] = colorFromFloat(saturationFScale);
        rgbRep[5] = colorFromFloat(valueFScale);

        return rgbRep;
    }

    static int rgbRepLength()
    {
        return 6;
    }
};

class PaletteColor
{ 
public:
    typedef std::array<float, 3> KMeansVector;

    // Create a color from an integer RGB value
    PaletteColor(QColor color);

    // Create a color from an integer RGB value
    PaletteColor(uint8_t r, uint8_t g, uint8_t b);

    // Use the group centroid and params to remap the color and store the result
    void SetRemapParams(const QColor& originalColorCentroid, const PaletteGroupRemapParams& groupParams);

    // Get the regular color
    QColor GetColor();

    // Get the remapped color
    QColor GetRemappedColor();

    static float normalizeH(float h);
    static float normalizeS(float s);
    static float normalizeV(float v);

    // kmeans convenience functions
    KMeansVector position();
    static float distance(const KMeansVector& a, const KMeansVector& b);
    static KMeansVector add(const PaletteColor::KMeansVector& a, const PaletteColor::KMeansVector& b);
    static KMeansVector scalarDivide(const PaletteColor::KMeansVector& a, float b);
    static void accumulate(PaletteColor::KMeansVector& a, const PaletteColor::KMeansVector& b);

    static float HueWeight;
    static  float SaturationWeight;
    static float ValueWeight;
    static bool UseCIEDistance;

private:
    QColor _color;
    QColor _remappedColor;
};

typedef std::vector<std::shared_ptr<PaletteColor>> ColorList;
typedef std::map<QRgb, std::shared_ptr<PaletteColor>> ColorMap;
typedef std::map<QRgb, PaletteColor*> TempColorMap;

void sortColorListByLuma(std::vector<std::shared_ptr<PaletteColor>>& colors);

inline ColorMap ColorMapFromColorList(const ColorList& list)
{
    ColorMap map;
    for (auto& color : list)
        map[color->GetColor().rgba()] = color;
    return map;
}

inline TempColorMap TempColorMapFromColorList(const ColorList& list)
{
    TempColorMap map;
    for (auto& color : list)
        map[color->GetColor().rgba()] = color.get();
    return map;
}

#endif // PALETTECOLOR_H
