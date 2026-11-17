// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "gameTypes/MapCoordinates.h"

class Minimap;

/// Übersichtskarte (MapPreview)
class ctrlMinimap : public Window
{
public:
    ctrlMinimap(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, const Extent& padding,
                const Extent& mapSize);

    Extent getCurMapSize() const { return drawnMapSize; }

    /// Get area the map covers (relative to control origin)
    Rect getMapArea() const;
    Rect getBoundaryRect() const override;
    Rect getMapDrawArea() const;

    void setPadding(const Extent& padding);
    /// Größe ändern
    void resize(const Extent& newSize) override;
    void setMapSize(const Extent& newMapSize);

    /// Liefert für einen gegebenen Map-Punkt die Pixel-Koordinaten relativ zur Bounding-Box
    DrawPoint calcMapCoord(MapPoint pt) const;

    /// Verkleinert Minimap soweit es geht (entfernt Bounding-Box)
    void removeBoundingBox(const Extent& minSize);

protected:
    /// Zeichnet die Minimap an sich
    void drawMap(Minimap& map) const;

    /// Real size of the minimap (gets scaled with retained aspect ratio)
    Extent drawnMapSize;
    /// Abstand der Minimap vom Rand des Controls
    Extent padding;
    /// Requested size of the drawn map
    Extent mapSize;
};
