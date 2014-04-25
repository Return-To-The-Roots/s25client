// $Id: customborderbuilder.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

class glArchivItem_Bitmap_RLE;

class CustomBorderBuilder
{
    public:
        CustomBorderBuilder(const libsiedler2::ArchivItem_Palette* const palette);
        ~CustomBorderBuilder();
        int loadEdges(const libsiedler2::ArchivInfo* archiveInfo);
        int buildBorder(const unsigned int width, const unsigned int height, libsiedler2::ArchivInfo* borderInfo);
        const libsiedler2::ArchivItem_Palette* palette;

    private:
        class BdrBitmap
        {
            public:
                BdrBitmap(const unsigned int width, const unsigned int height);
                ~BdrBitmap();
                BdrBitmap* get(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height) const;
                inline unsigned char get(const unsigned int x, const unsigned int y) const;
                void put(const unsigned int x, const unsigned int y, BdrBitmap* pic, bool picGetted = false); // mit true wird das übergebene BdrBitmap wieder zerstört. Das ist genau dann sinnvoll, wenn es mit BdrBitmap::get() erstellt wurde, da der Zeiger ja außeralb von BdrBitmap::put() nicht mehr verfügbar ist.
                inline void put(const unsigned int x, const unsigned int y, const unsigned char c);
                unsigned int w;
                unsigned int h;

            private:
                inline const unsigned int getpos(const unsigned int x, const unsigned int y) const;
                unsigned char* value;
        };

        void BitmapRLE2BdrBitmap(const glArchivItem_Bitmap_RLE* bitmapRLE, BdrBitmap* bdrBitmap);
        void BdrBitmap2BitmapRLE2(BdrBitmap* bdrBitmap, glArchivItem_Bitmap_RLE* bitmapRLE);

        void FindEdgeDistribution(unsigned int toFill, unsigned short lengths[3], unsigned char (counts)[3]);
        void WriteEdgeDistribution(const unsigned int x,
                                   const unsigned int y,
                                   const unsigned int toFill,
                                   const bool direction, // false = waagerecht, true = senkrecht
                                   const unsigned short edgeLengths[3],
                                   unsigned char edgeCounts[3], // wird verändert, nicht weiterbenutzen
                                   BdrBitmap* edges[],
                                   const unsigned char numFillers,
                                   BdrBitmap* fillers[],
                                   BdrBitmap* outBorder);

        bool edgesLoaded;
        static const unsigned char numCorners = 9;
        BdrBitmap* corners[numCorners];
        BdrBitmap* edgesTop[3]; // edges sind die "großen" Stücke, die jeweils zwischen zwei Auflösungen dazukommen.
        BdrBitmap* edgesBottom[3];
        BdrBitmap* edgesLeft[3];
        BdrBitmap* edgesRight[3];
        static const unsigned char numFillersTop = 4; // fillers sind zusammengesuchte "kleine" Stücke, die aneinandergereiht werden können
        BdrBitmap* fillersTop[numFillersTop];
        static const unsigned char numFillersBottom = 5;
        BdrBitmap* fillersBottom[numFillersBottom];
        static const unsigned char numFillersLeft = 5;
        BdrBitmap* fillersLeft[numFillersLeft];
        static const unsigned char numFillersRight = 6;
        BdrBitmap* fillersRight[numFillersRight];
};

#endif // CUSTOMBORDER_H_INCLUDED
