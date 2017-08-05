// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
#ifndef CUSTOMBORDERBUILDER_H_INCLUDED
#define CUSTOMBORDERBUILDER_H_INCLUDED

#pragma once
#include <boost/array.hpp>
#include <vector>

class glArchivItem_Bitmap;
class glArchivItem_Bitmap_RLE;

namespace libsiedler2 {
class ArchivItem_Palette;
class ArchivInfo;
} // namespace libsiedler2

class CustomBorderBuilder
{
public:
    CustomBorderBuilder(const libsiedler2::ArchivItem_Palette* const palette);
    ~CustomBorderBuilder();
    int loadEdges(const libsiedler2::ArchivInfo* archiveInfo);
    int buildBorder(const unsigned width, const unsigned height, boost::array<glArchivItem_Bitmap*, 4>& borderInfo);
    const libsiedler2::ArchivItem_Palette* palette;

private:
    class BdrBitmap
    {
    public:
        BdrBitmap(const unsigned width, const unsigned height);
        BdrBitmap* get(const unsigned x, const unsigned y, const unsigned width, const unsigned height) const;
        inline unsigned char get(const unsigned x, const unsigned y) const;
        void put(const unsigned x, const unsigned y, BdrBitmap* pic,
                 bool picGetted = false); // mit true wird das übergebene BdrBitmap wieder zerstört. Das ist genau dann sinnvoll, wenn es
                                          // mit BdrBitmap::get() erstellt wurde, da der Zeiger ja außeralb von BdrBitmap::put() nicht mehr
                                          // verfügbar ist.
        inline void put(const unsigned x, const unsigned y, const unsigned char c);
        unsigned w;
        unsigned h;

    private:
        inline const unsigned getpos(const unsigned x, const unsigned y) const;
        std::vector<unsigned char> values;
    };

    void BitmapRLE2BdrBitmap(const glArchivItem_Bitmap_RLE* bitmapRLE, BdrBitmap* bdrBitmap);
    void BdrBitmap2BitmapRLE2(BdrBitmap* bdrBitmap, glArchivItem_Bitmap_RLE* bitmapRLE);

    void FindEdgeDistribution(unsigned toFill, boost::array<unsigned short, 3>& lengths, boost::array<unsigned char, 3>& counts);
    template<size_t T_numEdges, size_t T_numFillers>
    void WriteEdgeDistribution(const unsigned x, const unsigned y, const unsigned toFill,
                               const bool direction, // false = waagerecht, true = senkrecht
                               const boost::array<unsigned short, 3>& edgeLengths,
                               boost::array<unsigned char, 3>& edgeCounts, // wird verändert, nicht weiterbenutzen
                               boost::array<BdrBitmap*, T_numEdges> edges, boost::array<BdrBitmap*, T_numFillers> fillers,
                               BdrBitmap* outBorder);

    bool edgesLoaded;
    static BOOST_CONSTEXPR_OR_CONST unsigned numCorners = 9;
    boost::array<BdrBitmap*, numCorners> corners;
    boost::array<BdrBitmap*, 3> edgesTop; // edges sind die "großen" Stücke, die jeweils zwischen zwei Auflösungen dazukommen.
    boost::array<BdrBitmap*, 3> edgesBottom;
    boost::array<BdrBitmap*, 3> edgesLeft;
    boost::array<BdrBitmap*, 3> edgesRight;
    static BOOST_CONSTEXPR_OR_CONST unsigned numFillersTop =
      4; // fillers sind zusammengesuchte "kleine" Stücke, die aneinandergereiht werden können
    boost::array<BdrBitmap*, numFillersTop> fillersTop;
    static BOOST_CONSTEXPR_OR_CONST unsigned numFillersBottom = 5;
    boost::array<BdrBitmap*, numFillersBottom> fillersBottom;
    static BOOST_CONSTEXPR_OR_CONST unsigned numFillersLeft = 5;
    boost::array<BdrBitmap*, numFillersLeft> fillersLeft;
    static BOOST_CONSTEXPR_OR_CONST unsigned numFillersRight = 6;
    boost::array<BdrBitmap*, numFillersRight> fillersRight;
};

#endif // CUSTOMBORDER_H_INCLUDED
