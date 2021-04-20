// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
Border-Builder 09.09.2008
by Stefan Kriwanek
contact@stefankriwanek.de

erstellt mit libsiedler2 rev 3935

Die Methode loadEdges() der Klasse lädt die zum Erstellen eines Rahmens nötigen Bitmaps in den Speicher.
Übergeben wird
archiveInfo:    Zeiger auf ein geladenes Archiv der RESOURCE.DAT/IDX

Die Methode buildBorder() erstellt einen Rahmen zu gegebener Größe.
Übergeben werden:
width, height:  die gewünschte Größe
borderInfo:     Zeiger auf ein initialisiertes, leeres Archiv

In borderInfo werden vier Bilder als glArchivItem_Bitmap_RLE an Index 0 bis 3 geschrieben, das sind die Rahmen oben,
unten, links und rechts, wobei die Ecken zu oben/unten gehören. Sie müssen also an den Stellen oben:   0        0 unten:
0        height-12 links:  0 12 rechts: widht-12 12 zu sehen sein.

Vor dem Aufruf von buildBorder() muss der interne, öffentliche Zeiger *palette auf ein ArchivItem_Palette* gesetzt
werden.
 */

#include "customborderbuilder.h"
#include "RttrForeachPt.h"
#include "ogl/glArchivItem_Bitmap_Direct.h"
#include "libsiedler2/Archiv.h"

CustomBorderBuilder::CustomBorderBuilder(const libsiedler2::ArchivItem_Palette& palette) : palette(palette)
{
    edgesLoaded = false;
    std::fill(edgesTop.begin(), edgesTop.end(), BdrBitmap());
    std::fill(edgesBottom.begin(), edgesBottom.end(), BdrBitmap());
    std::fill(edgesLeft.begin(), edgesLeft.end(), BdrBitmap());
    std::fill(edgesRight.begin(), edgesRight.end(), BdrBitmap());
    std::fill(fillersTop.begin(), fillersTop.end(), BdrBitmap());
    std::fill(fillersBottom.begin(), fillersBottom.end(), BdrBitmap());
    std::fill(fillersLeft.begin(), fillersLeft.end(), BdrBitmap());
    std::fill(fillersRight.begin(), fillersRight.end(), BdrBitmap());
}
CustomBorderBuilder::~CustomBorderBuilder() = default;

int CustomBorderBuilder::loadEdges(const libsiedler2::Archiv& archiveInfo)
{
    // simples Fehlerabfangen
    if(archiveInfo.size() != 57)
        return 1; // nicht RESOURCE.DAT übergeben

    // Musterstücke einladen
    /* Evtl. könnte man hier nur den größten Rahmen laden und alle Teile aus diesem rauskopieren, um das Ganze etwas
       schneller zu machen. Allerdings sind die einander entsprechenden Stücke teilweise nicht in jeder Auflösung
       tatsächlich gleich, sodass man das für jedes vorher prüfen müsste.*/
    BdrBitmap tempBMP(Extent(1280, 1024));
    // palette = dynamic_cast<glArchivItem_Bitmap_RLE*>(archiveInfo[4])->getPalette();
    // 640x480
    {
        Bitmap2BdrBitmap(dynamic_cast<const glArchivItem_Bitmap&>(*archiveInfo[4]), tempBMP);
        corners[0] = tempBMP.get(ImgPos(0, 0), Extent(584, 12));
        corners[1] = tempBMP.get(ImgPos(584, 0), Extent(56, 12));
        corners[2] = tempBMP.get(ImgPos(0, 468), Extent(202, 12));
        corners[3] = tempBMP.get(ImgPos(508, 468), Extent(132, 12));
        corners[4] = tempBMP.get(ImgPos(202, 468), Extent(306, 12));
        corners[5] = tempBMP.get(ImgPos(0, 12), Extent(12, 352));
        corners[6] = tempBMP.get(ImgPos(0, 364), Extent(12, 104));
        corners[7] = tempBMP.get(ImgPos(628, 12), Extent(12, 268));
        corners[8] = tempBMP.get(ImgPos(628, 280), Extent(12, 188));
        fillersTop[0] = tempBMP.get(ImgPos(270, 0), Extent(2, 12));
        fillersTop[2] = tempBMP.get(ImgPos(446, 0), Extent(42, 12));
        fillersTop[3] = tempBMP.get(ImgPos(166, 0), Extent(104, 12));
        fillersBottom[0] = tempBMP.get(ImgPos(508, 468), Extent(2, 12));
    }
    // 800x600
    {
        Bitmap2BdrBitmap(dynamic_cast<const glArchivItem_Bitmap&>(*archiveInfo[7]), tempBMP);
        edgesTop[0] = tempBMP.get(ImgPos(584, 0), Extent(160, 12));
        edgesBottom[0] = tempBMP.get(ImgPos(508, 588), Extent(160, 12));
        edgesLeft[0] = tempBMP.get(ImgPos(0, 364), Extent(12, 120));
        edgesRight[0] = tempBMP.get(ImgPos(788, 280), Extent(12, 120));
        fillersTop[1] = tempBMP.get(ImgPos(583, 0), Extent(32, 12));
    }
    // 1024x768
    {
        Bitmap2BdrBitmap(dynamic_cast<const glArchivItem_Bitmap&>(*archiveInfo[10]), tempBMP);
        edgesTop[1] = tempBMP.get(ImgPos(644, 0), Extent(224, 12));
        edgesBottom[1] = tempBMP.get(ImgPos(668, 756), Extent(224, 12));
        edgesLeft[1] = tempBMP.get(ImgPos(0, 484), Extent(12, 168));
        edgesRight[1] = tempBMP.get(ImgPos(1012, 400), Extent(12, 168));
        fillersBottom[1] = tempBMP.get(ImgPos(637, 756), Extent(23, 12));
        fillersBottom[2] = tempBMP.get(ImgPos(601, 756), Extent(34, 12));
        fillersBottom[3] = tempBMP.get(ImgPos(103, 756), Extent(42, 12));
        fillersBottom[4] = tempBMP.get(ImgPos(242, 756), Extent(72, 12));
        fillersLeft[0] = tempBMP.get(ImgPos(0, 524), Extent(12, 2));
        fillersLeft[2] = tempBMP.get(ImgPos(0, 222), Extent(12, 17));
        fillersLeft[3] = tempBMP.get(ImgPos(0, 423), Extent(12, 30));
        fillersLeft[4] = tempBMP.get(ImgPos(0, 114), Extent(12, 82));
        fillersRight[0] = tempBMP.get(ImgPos(1012, 280), Extent(12, 2));
        fillersRight[1] = tempBMP.get(ImgPos(1012, 281), Extent(12, 8));
        fillersRight[2] = tempBMP.get(ImgPos(1012, 528), Extent(12, 15));
        fillersRight[3] = tempBMP.get(ImgPos(1012, 291), Extent(12, 29));
        fillersRight[4] = tempBMP.get(ImgPos(1012, 475), Extent(12, 54));
        fillersRight[5] = tempBMP.get(ImgPos(1012, 320), Extent(12, 78));
    }
    // 1280x1024 links
    {
        BdrBitmap pic1(Extent(640, 1024));
        Bitmap2BdrBitmap(dynamic_cast<const glArchivItem_Bitmap&>(*archiveInfo[13]), pic1);
        tempBMP.put(ImgPos(0, 0), pic1);
    }
    // und rechts
    {
        BdrBitmap pic2(Extent(640, 1024));
        Bitmap2BdrBitmap(dynamic_cast<const glArchivItem_Bitmap&>(*archiveInfo[14]), pic2);
        tempBMP.put(ImgPos(pic2.size.x, 0), pic2);
        edgesTop[2] = tempBMP.get(ImgPos(968, 0), Extent(256, 12));
        edgesBottom[2] = tempBMP.get(ImgPos(892, 1012), Extent(256, 12));
        edgesLeft[2] = tempBMP.get(ImgPos(0, 652), Extent(12, 256));
        edgesRight[2] = tempBMP.get(ImgPos(1268, 568), Extent(12, 256));
        fillersLeft[1] = tempBMP.get(ImgPos(0, 769), Extent(12, 9));
    }

    edgesLoaded = true;
    return 0;
}

int CustomBorderBuilder::buildBorder(const Extent& size, std::array<glArchivItem_Bitmap*, 4>& borderInfo)
{
    // simples Fehlerabfangen
    if(size.x < 640 || size.y < 480)
        return 1; // kleiner geht nicht
    if(!edgesLoaded)
        return 2; // Die Stücken sind noch nicht geladen worden, so gehts nicht!

    // temporäre BdrBitmap's deklarieren
    std::array<BdrBitmap, 4> customEdges;
    customEdges[0] = BdrBitmap(Extent(size.x, 12));      // oben
    customEdges[1] = BdrBitmap(customEdges[0].size);     // unten
    customEdges[2] = BdrBitmap(Extent(12, size.y - 24)); // links
    customEdges[3] = BdrBitmap(customEdges[2].size);     // rechts

    // den Rahmen zusammenbauen
    {
        // Ecken werden einfach eingefügt
        // horizontale Ecken:
        ImgPos origin(0, 0);
        customEdges[0].put(origin, corners[0]);
        customEdges[0].put(ImgPos(size.x - 56, 0), corners[1]);
        customEdges[1].put(origin, corners[2]);
        customEdges[1].put(ImgPos(size.x - 132, 0), corners[3]);
        // das Mittelstück, damit das Bedienfeld passt
        customEdges[1].put(ImgPos(size.x / 2 - 118, 0), corners[4]);
        // vertikale Ecken:
        customEdges[2].put(origin, corners[5]);
        customEdges[2].put(ImgPos(0, size.y - 128), corners[6]);
        customEdges[3].put(origin, corners[7]);
        customEdges[3].put(ImgPos(0, size.y - 212), corners[8]);

        // Freie Flächen mit Kanten ausfüllen
        // Kanten
        unsigned toFillPixel;
        std::array<unsigned char, 3> countEdge = {{0, 0, 0}};
        std::array<unsigned short, 3> lengthEdge;
        // obere Kante
        ImgPos emptyFromPixel(584, 0);
        toFillPixel = size.x - 640;
        for(unsigned char i = 0; i < 3; i++)
            lengthEdge[i] = edgesTop[i].size.x;
        FindEdgeDistribution(toFillPixel, lengthEdge, countEdge);
        WriteEdgeDistribution(emptyFromPixel, toFillPixel, false, lengthEdge, countEdge, edgesTop, fillersTop,
                              customEdges[0]);
        // untere Kante links
        emptyFromPixel.x = 202;
        toFillPixel = size.x / 2 - 320;
        for(unsigned char i = 0; i < 3; i++)
            lengthEdge[i] = edgesBottom[i].size.x;
        FindEdgeDistribution(toFillPixel, lengthEdge, countEdge);
        WriteEdgeDistribution(emptyFromPixel, toFillPixel, false, lengthEdge, countEdge, edgesTop, fillersBottom,
                              customEdges[1]);
        // untere Kante rechts
        emptyFromPixel.x = size.x / 2 + 188;
        toFillPixel = size.x - size.x / 2
                      - 320; // hier steht w - w/2 statt w/2, um den Rundungsfehler bei ungeraden w zu kompensieren
        for(unsigned char i = 0; i < 3; i++)
            lengthEdge[i] = edgesBottom[i].size.x;
        FindEdgeDistribution(toFillPixel, lengthEdge, countEdge);
        WriteEdgeDistribution(emptyFromPixel, toFillPixel, false, lengthEdge, countEdge, edgesTop, fillersBottom,
                              customEdges[1]);
        // linke Kante
        emptyFromPixel = ImgPos(0, 352);
        toFillPixel = size.y - 480;
        for(unsigned char i = 0; i < 3; i++)
            lengthEdge[i] = edgesLeft[i].size.y;
        FindEdgeDistribution(toFillPixel, lengthEdge, countEdge);
        WriteEdgeDistribution(emptyFromPixel, toFillPixel, true, lengthEdge, countEdge, edgesLeft, fillersLeft,
                              customEdges[2]);
        // rechte Kante
        emptyFromPixel.y = 268;
        toFillPixel = size.y - 480;
        for(unsigned char i = 0; i < 3; i++)
            lengthEdge[i] = edgesRight[i].size.y;
        FindEdgeDistribution(toFillPixel, lengthEdge, countEdge);
        WriteEdgeDistribution(emptyFromPixel, toFillPixel, true, lengthEdge, countEdge, edgesRight, fillersRight,
                              customEdges[3]);
    }

    // Bildspeicher für Ausgaberahmen vorbereiten; in glArchivItem_Bitmap kovertieren
    for(unsigned i = 0; i < 4; i++)
    {
        glArchivItem_Bitmap* customEdgeBmp = new glArchivItem_Bitmap_Direct;
        customEdgeBmp->init(customEdges[i].size.x, customEdges[i].size.y, libsiedler2::TextureFormat::Paletted,
                            &palette);
        BdrBitmap2Bitmap(customEdges[i], *customEdgeBmp);
        borderInfo[i] = customEdgeBmp;
    }

    return 0;
}

void CustomBorderBuilder::Bitmap2BdrBitmap(const glArchivItem_Bitmap& bitmapRLE, BdrBitmap& bdrBitmap)
{
    RTTR_FOREACH_PT(ImgPos, bitmapRLE.GetSize())
        bdrBitmap.put(pt, bitmapRLE.getPixelClrIdx(pt.x, pt.y, &palette));
}

void CustomBorderBuilder::BdrBitmap2Bitmap(BdrBitmap& bdrBitmap, glArchivItem_Bitmap& bitmapRLE)
{
    RTTR_FOREACH_PT(ImgPos, bdrBitmap.size)
        bitmapRLE.setPixel(pt.x, pt.y, bdrBitmap.get(pt));
}

void CustomBorderBuilder::FindEdgeDistribution(unsigned toFill, std::array<unsigned short, 3>& lengths,
                                               std::array<unsigned char, 3>& shouldCounts)
{
    // Die should-Variablen speichern die bisher als am besten befundene Kombination; die would-Variablen die gerade zu
    // prüfende
    shouldCounts[0] = 0;
    shouldCounts[1] = 0;
    shouldCounts[2] = 0;
    std::array<unsigned char, 3> wouldCounts;
    std::array<unsigned char, 3> maxCounts; // wieviel mal passt jedes Teil maximal in die Freifläche?
    for(unsigned char i = 0; i < 3; i++)
        maxCounts[i] = toFill / lengths[i];
    unsigned shouldBeFilled = 0;
    unsigned char shouldNumDifferentTiles = 0;
    // Schleife über alle möglichen Kombinationen
    for(wouldCounts[0] = 0; wouldCounts[0] <= maxCounts[0]; wouldCounts[0]++)
        for(wouldCounts[1] = 0; wouldCounts[1] <= maxCounts[1]; wouldCounts[1]++)
            for(wouldCounts[2] = 0; wouldCounts[2] <= maxCounts[2]; wouldCounts[2]++)
            {
                // Finde, wieviel Platz die Kombination ausfüllen würde
                unsigned wouldBeFilled = 0;
                for(unsigned char i = 0; i < 3; i++)
                    wouldBeFilled += wouldCounts[i] * lengths[i];
                // wenn die Kombination nicht zu groß ist und weniger oder gleich viel Platz frei ließe als/wie bisher
                if(wouldBeFilled <= toFill)
                    if(wouldBeFilled >= shouldBeFilled)
                    {
                        // Finde, ob mehr verschiedene Stücken benutzt würden als bisher
                        unsigned char wouldNumDifferentTiles = 3;
                        for(unsigned char wouldCount : wouldCounts)
                            if(wouldCount == 0)
                                wouldNumDifferentTiles--;
                        // wenn mehr Stücke benutzt würden oder weniger Freifläche bleibt
                        if((wouldNumDifferentTiles > shouldNumDifferentTiles) || (wouldBeFilled > shouldBeFilled))
                        {
                            // Bessere Verteilung gefunden, in should-Variablen speichern
                            for(unsigned char i = 0; i < 3; i++)
                                shouldCounts[i] = wouldCounts[i];
                            shouldNumDifferentTiles = wouldNumDifferentTiles;
                            shouldBeFilled = wouldBeFilled;
                        }
                    }
            }
}

template<size_t T_numEdges, size_t T_numFillers>
void CustomBorderBuilder::WriteEdgeDistribution(const ImgPos& pos, const unsigned toFill,
                                                const bool direction, // false = waagerecht, true = senkrecht
                                                const std::array<unsigned short, 3>& edgeLengths,
                                                std::array<unsigned char, 3>& edgeCounts,
                                                const std::array<BdrBitmap, T_numEdges>& edges,
                                                const std::array<BdrBitmap, T_numFillers>& fillers,
                                                BdrBitmap& outBorder)
// Schreibt die übergebene Verteilung ins Bild und füllt die restliche Fläche auf.
{
    unsigned emptyFromPixel = direction ? pos.y : pos.x;
    unsigned toFillPixel = toFill;

    // Wieviele große Stücken zu schreiben?
    unsigned char edgesToDistribute = 0;
    for(unsigned char i = 0; i < 3; i++)
        edgesToDistribute += edgeCounts[i];

    // Der Reihe nach schreiben
    unsigned takeEdgeNum = 0;
    while(edgesToDistribute > 0)
    {
        if(edgeCounts[takeEdgeNum] > 0)
        {
            if(direction)
                outBorder.put(ImgPos(pos.x, emptyFromPixel), edges[takeEdgeNum]);
            else
                outBorder.put(ImgPos(emptyFromPixel, pos.y), edges[takeEdgeNum]);
            emptyFromPixel += edgeLengths[takeEdgeNum];
            toFillPixel -= edgeLengths[takeEdgeNum];
            edgeCounts[takeEdgeNum]--;
            edgesToDistribute--;
        }
        takeEdgeNum = (takeEdgeNum + 1) % 3;
    }

    // Fülle den Rest auf
    std::array<unsigned char, T_numFillers> numFillersToUse;
    std::fill(numFillersToUse.begin(), numFillersToUse.end(), 0);
    // Finde, wie oft jedes Füllerstück gebraucht wird. Einfach von groß nach klein einfügen, wenn der Platz jeweils
    // noch reicht.
    unsigned char curFiller = T_numFillers - 1;
    while(toFillPixel > 0)
    {
        unsigned short length = direction ? fillers[curFiller].size.y : fillers[curFiller].size.x;
        if(length <= toFillPixel)
        {
            numFillersToUse[curFiller]++;
            toFillPixel -= length;
        } else
        {
            if(curFiller > 0) // Das nächstkleinere Füllerstück testen
                curFiller--;
            else // Wenn am Ende weniger Platz freibleibt als das kleinste Füllerstück groß ist (2 Pixel), wird sofort
                 // ein passendes Teilstück eingefügt.
            {
                ImgPos origin(0, 0);
                ImgPos dstOffset;
                Extent size(12, toFillPixel);
                if(direction)
                    dstOffset = ImgPos(pos.x, emptyFromPixel);
                else
                {
                    std::swap(size.x, size.y);
                    dstOffset = ImgPos(emptyFromPixel, pos.y);
                }
                outBorder.put(dstOffset, fillers[0].get(origin, size));
                emptyFromPixel += toFillPixel;
                toFillPixel = 0;
            }
        }
    }

    // Schreibe die Füllerstückkombination von groß nach klein
    unsigned char numUsedFillers = 0;
    for(unsigned char i = 1; i < T_numFillers; i++)
        numUsedFillers += numFillersToUse[i];
    const Extent fillers0Size = fillers[0].size;
    for(unsigned char i = T_numFillers - 1; (i >= 1); i--) //
    {
        for(unsigned char j = 0; j < numFillersToUse[i]; j++)
        {
            // Vor jedem großen zuerst ein paar der kleinsten Füller, damit die großen nicht so aneinander gequetscht
            // sind.
            for(unsigned char k = 0; k < numFillersToUse[0] / (numUsedFillers + 1); k++)
            {
                if(direction)
                    outBorder.put(ImgPos(pos.x, emptyFromPixel), fillers[0]);
                else
                    outBorder.put(ImgPos(emptyFromPixel, pos.y), fillers[0]);
                emptyFromPixel += direction ? fillers0Size.y : fillers0Size.x;
            }
            if(direction)
                outBorder.put(ImgPos(pos.x, emptyFromPixel), fillers[i]);
            else
                outBorder.put(ImgPos(emptyFromPixel, pos.y), fillers[i]);
            emptyFromPixel += direction ? fillers[i].size.y : fillers[i].size.x;
        }
    }
    // Die restlichen kleinen Füller
    const unsigned char numSmallFillers =
      numFillersToUse[0] - (numFillersToUse[0] / (numUsedFillers + 1)) * numUsedFillers;
    for(unsigned char j = 0; j < numSmallFillers; j++)
    {
        if(direction)
            outBorder.put(ImgPos(pos.x, emptyFromPixel), fillers[0]);
        else
            outBorder.put(ImgPos(emptyFromPixel, pos.y), fillers[0]);
        emptyFromPixel += direction ? fillers[0].size.y : fillers[0].size.x;
    }
}

/********************************************* CustomBorderBuilder-Klasse ---*/

/*--- BdrBitmap-Klasse *******************************************************/
// public
CustomBorderBuilder::BdrBitmap::BdrBitmap(const Extent& size) : size(size)
{
    values.resize(prodOfComponents(size));
}

CustomBorderBuilder::BdrBitmap CustomBorderBuilder::BdrBitmap::get(const ImgPos& srcOffset,
                                                                   const Extent& targetSize) const
{
    BdrBitmap pic(targetSize);
    RTTR_FOREACH_PT(ImgPos, targetSize)
    {
        pic.put(pt, get(pt + srcOffset));
    }
    return pic;
}

unsigned char CustomBorderBuilder::BdrBitmap::get(const ImgPos& pos) const
{
    return values[getpos(pos)];
}

void CustomBorderBuilder::BdrBitmap::put(const ImgPos& dstOffset, const BdrBitmap& pic)
{
    RTTR_FOREACH_PT(ImgPos, pic.size)
    {
        put(pt + dstOffset, pic.get(pt));
    }
}

void CustomBorderBuilder::BdrBitmap::put(const ImgPos& pos, unsigned char c)
{
    values[getpos(pos)] = c;
}

unsigned CustomBorderBuilder::BdrBitmap::getpos(const ImgPos& pos) const
// liefert den Index eines Pixels (x,y) im internen Speicher value[]
{
    return pos.y * size.x + pos.x;
}
