#ifndef PALETTE_H
#define PALETTE_H

#include "palettegroup.h"
#include "palettecolor.h"
#include <QPixmap>
#include <map>

class Palette
{
public:

    Palette();

    // Scan through the provided image, adding any new pixel colors to the palette
    void AddImageColors(const QImage& image);

    // Scan the provided palette image for groups
    std::unique_ptr<QImage> SaveToPaletteImage(bool useRemap, bool saveMetadata, bool autosize, int width, int height);

    // Scan the provided palette image for groups an colors
    void LoadFromPaletteImage(const QImage& image, bool loadColors, bool loadGroups, bool loadParams);

    // Reset to a default state with no colors or groups
    void Reset();

    // Reset all group remaps to the control color
    void ResetRemaps();

    // Get a copy of the global color list (with all the nulls removed and no guarantee of order)
    ColorList GetAllColors();

    // Finds the groups that each of originalColors belongs to, removes them,
    // and appends them to the provided destination group.
    // This operation might result in the destruction of some groups if their
    // membership drops to zero!
    void AppendGroup(const ColorList& originalColors, std::shared_ptr<PaletteGroup> destGroup);

    // Finds the groups that each of originalColors belongs to, removes them,
    // and creates a new group with that membership. The group is added to the
    // internal groups list and returned.
    // This operation might result in the destruction of some groups if their
    // memebership drops to zero!
    std::shared_ptr<PaletteGroup> CreateGroup(const ColorList& originalColors);

    // Finds any unused colors and creates a group from them
    std::shared_ptr<PaletteGroup> CreateOrphanGroup();

    // Using the palette, change the provided index image to a color image
    // Use this only immediately after loading the palette
    std::unique_ptr<QImage> CreateColorImage(const QImage& indexImage);

    // Using the palette, transform the provided image into an 8 bit grey index image
    std::unique_ptr<QImage> CreateIndexImage(const QImage& colorImage, bool saveMetadata);

    // Using the palette, transform the provided index image into a full color image.
    std::unique_ptr<QImage> RecolorImage(const QImage& colorImage);

    // Using the palette, transform the provided index image into a full color image.
    std::unique_ptr<QImage> CreateControlImage(const QImage& colorImage);

    // Create an image that's transparent everywhere but the selected color
    std::unique_ptr<QImage> CreateSelectionOverlay(const QImage& colorImage, const ColorList& selectedColors);

    // Use k-means clustering to create groups from the colors
    size_t CreateGroupsByClustering(size_t numberOfGroups);

    // Fetch a palette group by ID
    std::shared_ptr<PaletteGroup> GetPaletteGroup(int groupID);

    // Fetch a palette group by a color it contains
    std::shared_ptr<PaletteGroup> GetPaletteGroupContainingColor(const QColor& originalColor);

    // Fetch a palette color by original color
    std::shared_ptr<PaletteColor> GetPaletteColor(const QColor& originalColor);
    PaletteColor* GetPaletteColorPtr(const QColor& originalColor);
    //PaletteColor* GetPaletteColorPtr(int colorIndex);

    // Gets the number of groups in this palette
    int GetPaletteGroupCount();
    size_t GetPaletteColorCount();

    // Gets a short 8 character MD5 hash that describes the palette structure.
    // This is very useful for distinguishing which index images go with which palettes.
    // Otherwise it's a big mess.
    QString GetPaletteStructureMD5(bool includeMetadata);

    bool IsEmpty();

private:
    ColorList getSpacedColorList();
    std::shared_ptr<PaletteColor> addPaletteColor(const QColor& color);
    void removeEmptyGroups();

    ColorList _colors; // A flat list of all colors
    std::map<QRgb, std::shared_ptr<PaletteColor>> _colorIndex; // easily map color values to the index where they are stored

    GroupList _groups;

};

#endif // PALETTE_H
