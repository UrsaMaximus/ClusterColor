#include "palette.h"
#include <QCryptographicHash>
#include <QBuffer>
#include<QRandomGenerator>
#include <math.h>
#include <iostream>
#include <algorithm>

Palette::Palette()
{

}

static int colorToIndex(QColor color)
{
    return color.red()+color.green()*256+color.blue()*256*256;
}

static QColor indexToColor(int index)
{
    return QColor(index%256, (index/256)%256, index/256/256, 255);
}

static bool isGroupDelimeter(const QColor& pxColor)
{
    return pxColor.red() == 255 && pxColor.green() == 255 && pxColor.blue() == 255 && pxColor.alpha() == 0;
}

static std::vector<QColor> readStripFromImage(const QImage& image, int baseX, int baseY, int offset, int length)
{
    int start = baseY * image.width() + baseX + offset;
    std::vector<QColor> strip(length);
    for (int i=0; i < length; i++)
    {
        int x = (i + start) % image.width();
        int y = (i + start) / image.width();
        strip[i] = image.pixelColor(x,y);
    }
    return strip;
}

static int scanForStripFromImage(const QImage& image, int offset, std::vector<QColor>& strip)
{
    int i = offset;
    int len = image.width()*image.height();

    while (i < len)
    {
        int x = i % image.width();
        int y = i / image.width();
        QColor pxColor = image.pixelColor(x,y);
        if (isGroupDelimeter(pxColor) && strip.size() > 0)
        {
            break;
        }
        else if (!isGroupDelimeter(pxColor))
        {
            strip.push_back(pxColor);
        }
        ++i;
    }

    return i;
}

static std::vector<std::vector<QColor>> readImageAsStrips(const QImage& image)
{
    int offset = 0;
    std::vector<std::vector<QColor>> strips;
    while (true)
    {
        std::vector<QColor> strip;
        offset = scanForStripFromImage(image, offset, strip);
        if (strip.size() > 0)
        {
            strips.push_back(std::move(strip));
        }
        else
        {
            break;
        }
    }
    return strips;
}

static void writeStripToImage(QImage& image, int baseX, int baseY, int offset, const std::vector<QColor>& strip )
{
    int start = baseY * image.width() + baseX + offset;
    for (int i=0; i < (int)strip.size(); i++)
    {
        int x = (i + start) % image.width();
        int y = (i + start) / image.width();
        image.setPixelColor(x,y, strip[i]);
    }
}

void Palette::Reset()
{
    _groups.clear();
    _colors.clear();
    _colorIndex.clear();
}

void Palette::ResetRemaps()
{
    for (auto& group : _groups)
    {
        group->SetRemapParams(PaletteGroupRemapParams());
    }
}

// Scan through the provided image, adding any new pixel colors to the palette
void Palette::AddImageColors(const QImage& image)
{
    for (int y=0; y < image.height(); y++)
    {
        for (int x=0; x < image.width(); x++)
        {
            QColor pxColor = image.pixelColor(x,y);
            if (pxColor.alpha() > 0)
            {
                if (_colorIndex.find(pxColor.rgba()) == _colorIndex.end())
                {
                    addPaletteColor(pxColor);
                }
            }
        }
    }
}

std::shared_ptr<PaletteColor> Palette::addPaletteColor(const QColor& pxColor)
{
    auto color = GetPaletteColor(pxColor);

    if (color == nullptr)
    {
        color = std::make_shared<PaletteColor>(pxColor);
        _colors.push_back(color);
        _colorIndex[pxColor.rgba()] = color;
     }

    return color;
}

// Scan the provided palette image for groups and colors
void Palette::LoadFromPaletteImage(const QImage& image, bool loadOriginal, bool loadParams)
{
    // Read the image in as strips
    std::vector<std::vector<QColor>> strips = readImageAsStrips(image);

    // Empty palette check
    if (strips.size() == 0)
        throw std::runtime_error("Palette contained no colors!");

    // Handle cases where we are loading legacy palettes without metadata
    int paletteGroups = strips.size()-1;
    if (paletteGroups <= 0)
    {
        paletteGroups = 1;
        loadParams = false;
    }

    // Color uniqueness check
    if (loadOriginal)
    {
        std::map<QRgb,bool> uniqueColors;
        for (int i=0; i < paletteGroups; ++i)
        {
            for (auto& color : strips[i])
            {
                if (color.alphaF() > 0) // Don't check uniqueness of transparent colors
                {
                    if (uniqueColors.find(color.rgba()) != uniqueColors.end())
                        throw std::runtime_error("Palette contains repeat colors! Original palettes may not contain repeat colors. Is this a remap palette?");
                    else
                        uniqueColors[color.rgba()] = true;
                }
            }
        }
    }

    // If we aren't going to be loading groups, then use the groups we have
    if (!loadOriginal)
        paletteGroups = _groups.size();

    // Metadata individual size check
    if (loadParams)
    {
        int metadataSizeInPalette = strips[strips.size()-1].size();
        if (metadataSizeInPalette % PaletteGroupRemapParams::rgbRepLength() != 0)
            throw std::runtime_error("Invalid metadata size!");

        // Metadata quantity check
        int metadataGroups = metadataSizeInPalette/PaletteGroupRemapParams::rgbRepLength();
        if (metadataGroups != paletteGroups)
            throw std::runtime_error("Metadata not present for every palette!");
    }

    // At this point we are cleared to load without crashing, so we can commit

    // Clear out everything
    if (loadOriginal)
    {
        Reset();

        for (int i=0; i < paletteGroups; ++i)
        {
            // Create a color list from the strip
            ColorList colors;

            for (auto& qcolor : strips[i])
            {
                if (qcolor.alphaF() > 0) // original colors MUST have an alpha above 0
                {
                    colors.push_back(addPaletteColor(qcolor));
                }
            }

            // Create a group for this color list (even if it's empty!)
            CreateGroup(colors);
        }
    }

    // If we are meant to be loading in params, then parse the metadata
    // and apply it to the groups
    if (loadParams)
    {
        // Read in the metadata from the final strip
        for (int i=0; i < paletteGroups; ++i)
        {
            auto itrStart = strips[strips.size()-1].begin()+i*PaletteGroupRemapParams::rgbRepLength();
            auto itrEnd = itrStart + PaletteGroupRemapParams::rgbRepLength();
            PaletteGroupRemapParams p(std::vector<QColor>(itrStart, itrEnd));

            // Figure out if the transparency box is checked
            if (strips[i].size() > 0)
            {
                p.transparent = (strips[i][0].alphaF() == 0);
            }

            _groups[i]->SetRemapParams(p);
        }
    }

    // There might be empty groups, get rid of them
    removeEmptyGroups();

    // At the end of this process we might have loaded colors with no group memberships
    CreateOrphanGroup();
}

void Palette::removeEmptyGroups()
{
    auto groups = _groups;
    _groups.clear();
    for (auto& group : groups)
    {
        if (group->GetColorCount() > 0)
            _groups.push_back(group);
    }
}

ColorList Palette::getSpacedColorList()
{
    ColorList spaced;
    for (auto& group : _groups)
    {
        ColorList colors = group->GetAllColors();
        for (auto& color : colors)
        {
            spaced.push_back(color);
        }
        spaced.push_back(nullptr);
    }
    return spaced;
}

std::unique_ptr<QImage> Palette::SaveToPaletteImage(bool useRemap, bool saveMetadata, bool autosize, int width, int height)
{
    // Because color indices are going to really matter here, fixup the colors list
    ColorList spacedColors;
    if (saveMetadata)
    {
        spacedColors = getSpacedColorList();
    }
    ColorList& colors = saveMetadata ? spacedColors : _colors;

    // Get the metadata size and the overall size
    int metadataSize = saveMetadata ? PaletteGroupRemapParams::rgbRepLength() * _groups.size() : 0;
    int size = colors.size() + metadataSize;

    if (autosize)
    {
        width = 16;
        height = 1;
        int exp = 0;
        while (width * height < size)
        {
            exp++;
            height = pow(2, exp);
        }
    }
    else if (width * height < size)
    {
        throw std::runtime_error("Cannot create palette! Fixed palette size is too small.");
    }

    std::unique_ptr<QImage> paletteImage = std::make_unique<QImage>(width, height, QImage::Format::Format_ARGB32);
    paletteImage->fill(QColor(255,255,255,0));

    int colorIndex = 0;

    for (int y=0; y < height; y++)
    {
        for (int x=0; x < width; x++)
        {
            if (colorIndex >= (int)colors.size())
                break;

            if (colors[colorIndex] == nullptr)
            {
                paletteImage->setPixelColor(x,y, QColor(255,255,255,0));
            }
            else
            {
                if (useRemap)
                    paletteImage->setPixelColor(x,y, colors[colorIndex]->GetRemappedColor());
                else
                    paletteImage->setPixelColor(x,y, colors[colorIndex]->GetColor());
            }

            colorIndex++;

        }
        if (colorIndex > (int)colors.size())
            break;
    }

    if (saveMetadata)
    {
        // Write our metadata to the image
        int offset = paletteImage->width()*paletteImage->height()-(_groups.size() * PaletteGroupRemapParams::rgbRepLength());
        for (int i = 0; i < (int)_groups.size(); i++)
        {
            writeStripToImage(*paletteImage, 0, 0, offset, _groups[i]->GetRemapParams().createRgbRep());
            offset+= PaletteGroupRemapParams::rgbRepLength();
        }
    }

    return paletteImage;
}

std::shared_ptr<PaletteGroup> Palette::CreateOrphanGroup()
{
    ColorList orphans;
    for (auto& color : _colors)
    {
        if (color != nullptr && GetPaletteGroupContainingColor(color->GetColor()) == nullptr)
        {
            orphans.push_back(color);
        }
    }

    if (orphans.size() > 0)
        return CreateGroup(orphans);

    return nullptr;
}

std::unique_ptr<QImage> Palette::CreateColorImage(const QImage& indexImage)
{
    // Because color indices are going to really matter here, fixup the colors list
    ColorList colors = getSpacedColorList();

    std::unique_ptr<QImage> colorImage = std::make_unique<QImage>(indexImage.width(), indexImage.height(), QImage::Format::Format_ARGB32);
    colorImage->fill(QColor(0,0,0,0));

    for (int y=0; y < indexImage.height(); y++)
    {
        for (int x=0; x < indexImage.width(); x++)
        {
            QColor indexColor = indexImage.pixelColor(x,y);

            if (indexColor.alpha() > 0)
            {
                int index = colorToIndex(indexColor);

                PaletteColor* color;
                if (index < 0 || index >= (int)colors.size())
                    color = nullptr;
                else
                    color = colors[index].get();

                if (color != nullptr)
                {
                    colorImage->setPixelColor(x, y, color->GetColor());
                }
                else
                {
                    colorImage->setPixelColor(x, y, QColor(0,0,0,255));
                }
            }
        }
    }

    return colorImage;
}

std::unique_ptr<QImage> Palette::CreateIndexImage(const QImage& colorImage, bool saveMetadata )
{
    // Because color indices are going to really matter here, fixup the colors list
    ColorList spacedColors;
    if (saveMetadata)
    {
        spacedColors = getSpacedColorList();
    }
    ColorList& colors = saveMetadata ? spacedColors : _colors;

    // Make a backwards map for the sake of efficiency
    std::map<QRgb, int> indexFromColor;
    for (int i=0; i < (int)colors.size(); i++)
    {
        if (colors[i] != nullptr)
        {
            indexFromColor[colors[i]->GetColor().rgba()] = i;
        }
    }

    std::unique_ptr<QImage> indexImage = std::make_unique<QImage>(colorImage.width(), colorImage.height(), QImage::Format::Format_ARGB32);
    indexImage->fill(QColor(0,0,0,0));

    for (int y=0; y < colorImage.height(); y++)
    {
        for (int x=0; x < colorImage.width(); x++)
        {
            QColor pxColor = colorImage.pixelColor(x,y);

            if (pxColor.alpha() > 0)
            {
                int index = 0;
                if (indexFromColor.find(pxColor.rgba()) != indexFromColor.end())
                {
                    index = indexFromColor[pxColor.rgba()];
                }
                indexImage->setPixelColor(x, y, indexToColor(index));
            }
        }
    }

    return indexImage;
}

// Using the palette, transform the provided index image into a full color image.
// Use the remapped colors if requested.
std::unique_ptr<QImage> Palette::RecolorImage(const QImage& colorImage)
{
    std::unique_ptr<QImage> recolorImage = std::make_unique<QImage>(colorImage.width(), colorImage.height(), QImage::Format::Format_ARGB32);
    recolorImage->fill(QColor(0,0,0,0));

    for (int y=0; y < colorImage.height(); y++)
    {
        for (int x=0; x < colorImage.width(); x++)
        {
            QColor originalColor = colorImage.pixelColor(x,y);

            if (originalColor.alpha() > 0)
            {
                PaletteColor* remappedColor = GetPaletteColorPtr(originalColor);

                if (remappedColor != nullptr)
                {
                    recolorImage->setPixelColor(x, y, remappedColor->GetRemappedColor());
                }
                else
                {
                    recolorImage->setPixelColor(x, y, QColor(0,0,0,255));
                }
            }
        }
    }

    return recolorImage;
}

// Using the palette, transform the provided index image into a full color image.
// Use the remapped colors if requested.
std::unique_ptr<QImage> Palette::CreateControlImage(const QImage& colorImage)
{
    std::unique_ptr<QImage> controlImage = std::make_unique<QImage>(colorImage.width(), colorImage.height(), QImage::Format::Format_ARGB32);
    controlImage->fill(QColor(0,0,0,0));

    for (int y=0; y < colorImage.height(); y++)
    {
        for (int x=0; x < colorImage.width(); x++)
        {
            QColor originalColor = colorImage.pixelColor(x,y);

            if (originalColor.alpha() > 0)
            {
                PaletteColor* remappedColor = GetPaletteColorPtr(originalColor);

                if (remappedColor != nullptr)
                {
                    for (auto& group : _groups)
                    {
                        if (group->ContainsColor(remappedColor->GetColor()))
                        {
                            controlImage->setPixelColor(x, y, group->GetOriginalColorCentroid());
                            break;
                        }
                    }
                }
            }
        }
    }

    return controlImage;
}

// Using the palette, transform the provided index image into a full color image.
// Use the remapped colors if requested.
std::unique_ptr<QImage> Palette::CreateSelectionOverlay(const QImage& colorImage, const ColorList& selectedColors)
{
    if (selectedColors.size()==0)
        return nullptr;

    if (!selectedColors[0]->GetColor().isValid())
        return nullptr;

    std::unique_ptr<QImage> overlayImage = std::make_unique<QImage>(colorImage.width(), colorImage.height(), QImage::Format::Format_ARGB32);
    overlayImage->fill(QColor(0,0,0,0));

    float h = PaletteColor::normalizeH(selectedColors[0]->GetColor().hueF() + 0.5);
    float s = PaletteColor::normalizeS(selectedColors[0]->GetColor().saturationF());
    float v = PaletteColor::normalizeV(selectedColors[0]->GetColor().valueF() < 0.5 ? selectedColors[0]->GetColor().valueF() + 0.5f : selectedColors[0]->GetColor().valueF() - 0.5f);

    QColor selectedColorHighlight = QColor::fromHsvF(h,s,v).convertTo(QColor::Spec::Rgb);

    //create a temporary membership map to speed up the selection overlay creation
    auto map = TempColorMapFromColorList(selectedColors);

    for (int y=0; y < colorImage.height(); y++)
    {
        for (int x=0; x < colorImage.width(); x++)
        {
            QColor pxColor = colorImage.pixelColor(x,y);
            auto selColItr = map.find(pxColor.rgba());

            if (selColItr != map.end())
            {
                overlayImage->setPixelColor(x, y, selectedColorHighlight);
            }
        }
    }

    return overlayImage;
}

// Use k-means clustering to create groups from the colors
// returns the actual number of groups created
size_t Palette::CreateGroupsByClustering(size_t numberOfGroups)
{
    // Get a list of all the colors without nulls
    ColorList colors = GetAllColors();

    if (numberOfGroups > colors.size())
        numberOfGroups = colors.size();

    if (numberOfGroups == 0)
        return 0;

    int maxIterations = 6;

    // Create some vectors we'll need for kmeans

    std::vector<int> numberOfMembers(numberOfGroups, 0);
    std::vector<size_t> groupMembership(colors.size(), -1);
    std::vector<PaletteColor::KMeansVector> groupCentroids(numberOfGroups);

    // Select some initial points (this part is a lot harder than it looks!)
    int icc = 0;
    while (icc < (int)numberOfGroups)
    {
        if ((int)_groups.size() > icc)
        {
            groupCentroids[icc] = _groups[icc]->GetAllColors()[0]->position();  // TBD: this is not great
            icc++;
        }
        else
        {
            // Find the color with the highest minimum distance from any already chosen centroid
            float globalMinDist = 0;
            PaletteColor::KMeansVector candidatePos;

            for (int i=0; i < (int)colors.size(); i++)
            {
                float colorMin = 99999999999.9;
                for (int j=0; j < icc; j++)
                {
                    float distToCentroid = PaletteColor::distance(colors[i]->position(), groupCentroids[j]);
                    if (distToCentroid < colorMin)
                    {
                        colorMin = distToCentroid;
                    }
                }

                if (colorMin > globalMinDist)
                {
                    globalMinDist = colorMin;
                    candidatePos = colors[i]->position();
                }
            }

            groupCentroids[icc] = candidatePos;
            icc++;
        }
    }

    for (int iter=0; iter < maxIterations; iter++)
    {
        // Assign group memberships
        for (size_t i = 0; i < colors.size(); i++)
        {
            // Set this color's membership tenatively to group 0
            groupMembership[i] = 0;
            float minDist = PaletteColor::distance(groupCentroids[0], colors[i]->position());

            // Check all the other groups for better qualified membership
            for (size_t groupID = 1; groupID < numberOfGroups; groupID++)
            {
                float dist = PaletteColor::distance(groupCentroids[groupID], colors[i]->position());
                if (dist < minDist)
                {
                    groupMembership[i] = groupID;
                    minDist = dist;
                }
            }
        }

        // Set all centroids to 0,0,0 and member counts to zero
        for (size_t i = 0; i < numberOfGroups; i++)
        {
            groupCentroids[i] = PaletteColor::KMeansVector();
            numberOfMembers[i] = 0;
        }

        // Add up all the vectors and keep track of membership counts
        for (size_t i = 0; i < colors.size(); i++)
        {
            PaletteColor::accumulate(groupCentroids[groupMembership[i]], colors[i]->position());
            numberOfMembers[groupMembership[i]]++;
        }

        // Divide the sum of all verctors by the membership count to get the new centroid
        for (size_t i = 0; i < numberOfGroups; i++)
        {
            groupCentroids[i] = PaletteColor::scalarDivide(groupCentroids[i], (float)numberOfMembers[i]);
        }
    }

    // Construct groups based on the current memberships
    std::vector<std::shared_ptr<PaletteGroup>> groups;
    for (size_t groupID = 0; groupID < numberOfGroups; groupID++)
    {
        ColorList groupMembers;

        for (size_t i=0; i < colors.size(); i++)
        {
            if (groupMembership[i] == groupID)
            {
                groupMembers.push_back(colors[i]);
            }
        }

        if (groupMembers.size() > 0)
            groups.push_back(std::make_shared<PaletteGroup>(groupMembers));
    }

    // Assign the constructed groups list to the object
    _groups = groups;

    // Return the actual number of populated groups
    return _groups.size();
}

void Palette::AppendGroup(const ColorList& originalColors, std::shared_ptr<PaletteGroup> destGroup)
{
    // Remove all colors from groups if they are there
    for (auto&  color : originalColors)
    {
        // Find the group this color belongs to
        std::shared_ptr<PaletteGroup> group = GetPaletteGroupContainingColor(color->GetColor());

        // Don't remove colors if they're already in the correct group
        if (group != nullptr && group != destGroup)
        {
            // Check if it's already isolated
            if (group->GetAllColors().size() == 1)
            {
                _groups.erase(std::find(_groups.begin(), _groups.end(), group));
            }
            else
            {
                // Remove the color from the palette
                group->RemoveColor(color);
            }
        }
    }

    // Add all the now-freed colors to their new group
    // If the color is already in the group, add color silently ignores it
    for (auto& color : originalColors)
        destGroup->AddColor(color);
}

ColorList  Palette::GetAllColors()
{
    return _colors;
}

std::shared_ptr<PaletteGroup> Palette::CreateGroup(const ColorList& originalColors)
{
    // Remove all colors from groups if they are there
    for (auto&  color : originalColors)
    {
        // Find the group this color belongs to
        std::shared_ptr<PaletteGroup> group = GetPaletteGroupContainingColor(color->GetColor());

        if (group != nullptr)
        {
            // Check if it's already isolated
            if (group->GetAllColors().size() == 1)
            {
                _groups.erase(std::find(_groups.begin(), _groups.end(), group));
            }
            else
            {
                // Remove the color from the palette
                group->RemoveColor(color);
            }
        }
    }

    // Construct a new palette with just this color
    auto newGroup = std::make_shared<PaletteGroup>(originalColors);
    _groups.push_back(newGroup);

    return newGroup;
}

std::shared_ptr<PaletteGroup> Palette::GetPaletteGroupContainingColor(const QColor& originalColor)
{
    for (auto& group : _groups)
    {
        if (group->ContainsColor(originalColor))
            return group;
    }
    return nullptr;
}

std::shared_ptr<PaletteColor> Palette::GetPaletteColor(const QColor& originalColor)
{
    if (_colorIndex.find(originalColor.rgba()) == _colorIndex.end())
        return nullptr;
    return _colorIndex[originalColor.rgba()];
}

PaletteColor* Palette::GetPaletteColorPtr(const QColor& originalColor)
{
    auto mapItr = _colorIndex.find(originalColor.rgba());
    if (mapItr == _colorIndex.end())
        return nullptr;
    return mapItr->second.get();
}

int Palette::GetPaletteGroupCount()
{
    return (int)_groups.size();
}

// Fetch a palette group by ID
std::shared_ptr<PaletteGroup> Palette::GetPaletteGroup(int groupID)
{
    return _groups[groupID];
}

size_t Palette::GetPaletteColorCount()
{
    return _colors.size();
}

QString Palette::GetPaletteStructureMD5(bool includeMetadata)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);

    out << includeMetadata;
    out << (int)_groups.size();
    for (auto& group : _groups)
    {
        out << (int)group->GetAllColors().size();
        out << (int)group->GetOriginalColorCentroid().rgba();
    }

    QByteArray hash = QCryptographicHash::hash(buffer.data(), QCryptographicHash::Md5);
    QString hashStr = QString(hash.toHex().toUpper());
    hashStr.truncate(8);
    return hashStr;
}

bool Palette::IsEmpty()
{
    return _colors.size() == 0;
}
