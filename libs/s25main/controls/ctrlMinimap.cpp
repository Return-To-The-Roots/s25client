// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlMinimap.h"
#include "Minimap.h"
#include "gameData/MinimapConsts.h"

ctrlMinimap::ctrlMinimap(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size,
                         const Extent& padding, const Extent& mapSize)
    : Window(parent, id, pos, size), padding(padding), mapSize(mapSize)
{
    SetMapSize(mapSize);
}

Rect ctrlMinimap::GetMapArea() const
{
    return Rect(DrawPoint(GetSize() - drawnMapSize) / 2, drawnMapSize);
}

Rect ctrlMinimap::GetBoundaryRect() const
{
    Rect mapArea = GetMapDrawArea();
    mapArea.setSize(mapArea.getSize() + padding);
    return mapArea;
}

Rect ctrlMinimap::GetMapDrawArea() const
{
    return Rect::move(GetMapArea(), GetDrawPos());
}

void ctrlMinimap::SetPadding(const Extent& padding)
{
    this->padding = padding;
    SetMapSize(mapSize);
}

void ctrlMinimap::Resize(const Extent& newSize)
{
    Window::Resize(newSize);
    SetMapSize(mapSize);
}

void ctrlMinimap::SetMapSize(const Extent& newMapSize)
{
    mapSize = newMapSize;

    // Drawn map size is the full size reduced by the padding but >= 0
    drawnMapSize = Extent(elMax(GetSize() - 2 * Position(padding), Position::all(0)));
    // If any size is 0 -> no map
    if(prodOfComponents(mapSize) == 0 || prodOfComponents(drawnMapSize) == 0)
        drawnMapSize = Extent::all(0);
    else
    {
        // Rescale width due to geometry using triangles
        double scaled_map_width = mapSize.x * MINIMAP_SCALE_X;

        // Find out scale factors to due a box fit while retaining aspect ratio
        RTTR_Assert(mapSize.y != 0);
        RTTR_Assert(scaled_map_width != 0.); //-V550
        double x_scale = double(drawnMapSize.x) / scaled_map_width;
        double y_scale = double(drawnMapSize.y) / double(mapSize.y);

        // Scale by the smaller factor
        // Note: Only 1 coord needs to be scaled. The other is already the max size
        if(y_scale <= x_scale)
            drawnMapSize.x = static_cast<unsigned>(scaled_map_width * y_scale);
        else
            drawnMapSize.y = static_cast<unsigned>(mapSize.y * x_scale);
    }
}

DrawPoint ctrlMinimap::CalcMapCoord(MapPoint pt) const
{
    DrawPoint result = GetMapArea().getOrigin();
    result += DrawPoint(drawnMapSize * Extent(pt) / mapSize);
    return result;
}

void ctrlMinimap::DrawMap(Minimap& map) const
{
    // Map an sich zeichnen
    map.Draw(GetMapDrawArea());
}

void ctrlMinimap::RemoveBoundingBox(const Extent& minSize)
{
    Extent newSize = drawnMapSize + padding * 2u;
    newSize = elMax(newSize, minSize);
    padding = Extent(0, 0);
    Resize(newSize);
}
