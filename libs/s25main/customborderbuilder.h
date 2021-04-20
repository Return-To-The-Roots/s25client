// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "Point.h"
#include <array>
#include <vector>

class glArchivItem_Bitmap;

namespace libsiedler2 {
class ArchivItem_Palette;
class Archiv;
} // namespace libsiedler2

/// Position in an image
using ImgPos = Point<unsigned>;

class CustomBorderBuilder
{
public:
    CustomBorderBuilder(const libsiedler2::ArchivItem_Palette& palette);
    ~CustomBorderBuilder();
    int loadEdges(const libsiedler2::Archiv& archiveInfo);
    int buildBorder(const Extent& size, std::array<glArchivItem_Bitmap*, 4>& borderInfo);
    const libsiedler2::ArchivItem_Palette& palette;

private:
    class BdrBitmap
    {
    public:
        BdrBitmap() : size(0, 0) {}
        BdrBitmap(const Extent& size);
        BdrBitmap get(const ImgPos& srcOffset, const Extent& targetSize) const;
        unsigned char get(const ImgPos& pos) const;
        void put(const ImgPos& dstOffset, const BdrBitmap& pic);
        void put(const ImgPos& pos, unsigned char c);
        Extent size;

    private:
        unsigned getpos(const ImgPos& pos) const;
        std::vector<unsigned char> values;
    };

    void Bitmap2BdrBitmap(const glArchivItem_Bitmap& bitmapRLE, BdrBitmap& bdrBitmap);
    static void BdrBitmap2Bitmap(BdrBitmap& bdrBitmap, glArchivItem_Bitmap& bitmapRLE);

    static void FindEdgeDistribution(unsigned toFill, std::array<unsigned short, 3>& lengths,
                                     std::array<unsigned char, 3>& shouldCounts);
    template<size_t T_numEdges, size_t T_numFillers>
    void WriteEdgeDistribution(const ImgPos& pos, unsigned toFill,
                               bool direction, // false = waagerecht, true = senkrecht
                               const std::array<unsigned short, 3>& edgeLengths,
                               std::array<unsigned char, 3>& edgeCounts, // wird verändert, nicht weiterbenutzen
                               const std::array<BdrBitmap, T_numEdges>& edges,
                               const std::array<BdrBitmap, T_numFillers>& fillers, BdrBitmap& outBorder);

    bool edgesLoaded;
    static constexpr unsigned numCorners = 9;
    std::array<BdrBitmap, numCorners> corners;
    std::array<BdrBitmap, 3>
      edgesTop; // edges sind die "großen" Stücke, die jeweils zwischen zwei Auflösungen dazukommen.
    std::array<BdrBitmap, 3> edgesBottom;
    std::array<BdrBitmap, 3> edgesLeft;
    std::array<BdrBitmap, 3> edgesRight;
    static constexpr unsigned numFillersTop =
      4; // fillers sind zusammengesuchte "kleine" Stücke, die aneinandergereiht werden können
    std::array<BdrBitmap, numFillersTop> fillersTop;
    static constexpr unsigned numFillersBottom = 5;
    std::array<BdrBitmap, numFillersBottom> fillersBottom;
    static constexpr unsigned numFillersLeft = 5;
    std::array<BdrBitmap, numFillersLeft> fillersLeft;
    static constexpr unsigned numFillersRight = 6;
    std::array<BdrBitmap, numFillersRight> fillersRight;
};
