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

#include "defines.h" // IWYU pragma: keep
#include "ctrlMinimap.h"
#include "Minimap.h"
#include "gameData/MinimapConsts.h"

ctrlMinimap::ctrlMinimap(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size, const Extent& padding,
                         const Extent& mapSize)
    : Window(parent, id, pos, size), padding(padding), mapSize(mapSize), useBoundingBox(true)
{
    SetMapSize(mapSize);
}

Rect ctrlMinimap::GetMapArea() const
{
    return Rect(DrawPoint(GetSize() - curMapSize) / 2, curMapSize);
}

Rect ctrlMinimap::GetBoundaryRect() const
{
    return GetMapDrawArea();
}

Rect ctrlMinimap::GetMapDrawArea() const
{
    return Rect::move(GetMapArea(), GetDrawPos());
}

void ctrlMinimap::Resize(const Extent& newSize)
{
    Window::Resize(newSize);
    SetMapSize(useBoundingBox ? mapSize : newSize);
}

void ctrlMinimap::SetMapSize(const Extent& newMapSize)
{
    mapSize = Extent(std::max(newMapSize.x, padding.x * 2u), std::max(newMapSize.y, padding.y * 2u));

    if(!useBoundingBox)
        return;

    unsigned scaled_map_width = static_cast<unsigned>(mapSize.x * MINIMAP_SCALE_X);
    double x_scale = double(scaled_map_width) / double(mapSize.x - padding.x * 2);
    double y_scale = double(mapSize.y) / double(mapSize.y - padding.y * 2);

    bool scale_width = false;

    scale_width = x_scale <= y_scale;

    RTTR_Assert(mapSize.y != 0);
    RTTR_Assert(scaled_map_width != 0);

    curMapSize = GetSize() - padding * 2u;
    if(scale_width)
        curMapSize.x = (scaled_map_width * curMapSize.y / mapSize.y);
    else
        curMapSize.y = mapSize.y * curMapSize.x / scaled_map_width;
}

DrawPoint ctrlMinimap::CalcMapCoord(MapPoint pt) const
{
    DrawPoint result = GetMapArea().getOrigin();
    result += DrawPoint(curMapSize * Extent(pt) / mapSize);
    return result;
}

void ctrlMinimap::DrawMap(Minimap& map)
{
    // Map an sich zeichnen
    map.Draw(GetMapDrawArea());
}

void ctrlMinimap::RemoveBoundingBox(const Extent& minSize)
{
    if(!useBoundingBox)
        return;
    useBoundingBox = false;
    Extent newSize = curMapSize + padding * 2u;
    newSize = elMax(newSize, minSize);
    padding = Extent(0, 0);
    Resize(newSize);
}
