// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "Window.h"
#include "gameTypes/MapCoordinates.h"

class Minimap;

/// Übersichtskarte (MapPreview)
class ctrlMinimap : public Window
{
public:
    ctrlMinimap(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, const Extent& padding, const Extent& mapSize);

    Extent GetCurMapSize() const { return drawnMapSize; }

    /// Get area the map covers (relative to control origin)
    Rect GetMapArea() const;
    Rect GetBoundaryRect() const override;
    Rect GetMapDrawArea() const;

    void SetPadding(const Extent& padding);
    /// Größe ändern
    void Resize(const Extent& newSize) override;
    void SetMapSize(const Extent& newMapSize);

    /// Liefert für einen gegebenen Map-Punkt die Pixel-Koordinaten relativ zur Bounding-Box
    DrawPoint CalcMapCoord(MapPoint pt) const;

    /// Verkleinert Minimap soweit es geht (entfernt Bounding-Box)
    void RemoveBoundingBox(const Extent& minSize);

protected:
    /// Zeichnet die Minimap an sich
    void DrawMap(Minimap& map);

    /// Real size of the minimap (gets scaled with retained aspect ratio)
    Extent drawnMapSize;
    /// Abstand der Minimap vom Rand des Controls
    Extent padding;
    /// Requested size of the drawn map
    Extent mapSize;
};
