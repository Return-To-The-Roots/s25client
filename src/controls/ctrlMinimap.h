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
#ifndef CTRL_MINIMAP_H_
#define CTRL_MINIMAP_H_

#include "Window.h"
#include "gameTypes/MapTypes.h"

class Minimap;

/// Übersichtskarte (MapPreview)
class ctrlMinimap : public Window
{
    public:

        ctrlMinimap( Window* parent,
                     const unsigned int id,
                     const unsigned short x,
                     const unsigned short y,
                     const unsigned short width,
                     const unsigned short height,
                     const unsigned short padding_x,
                     const unsigned short padding_y,
                     const unsigned short map_width,
                     const unsigned short map_height);

        /// Gibt width_show und height_show zurück
        unsigned short GetWidthShow() const { return width_show; }
        unsigned short GetHeightShow() const { return height_show; }

        /// Gibt die entsprechenden Kanten relativ zur oberen linken Ecke der Bounding-Box
        DrawPoint GetBBOffset() const { return DrawPoint(GetLeft(), GetTop()); }
        unsigned short GetLeft() const { return (width_ - width_show) / 2; }
        unsigned short GetTop() const { return (height_ - height_show) / 2; }
        unsigned short GetRight() const { return GetLeft() + width_show; }
        unsigned short GetBottom() const { return GetTop() + height_show; }

        /// Größe ändern
        void Resize(unsigned short width, unsigned short heigth) override;
        void SetMapSize(const unsigned short map_width, const unsigned short map_height);

        /// Liefert für einen gegebenen Map-Punkt die Pixel-Koordinaten relativ zur Bounding-Box
        DrawPoint CalcMapCoord(MapPoint pt) const;

        /// Verkleinert Minimap soweit es geht (entfernt Bounding-Box)
        void RemoveBoundingBox(const unsigned short width_min, const unsigned short height_min);

    protected:

        /// Zeichnet die Minimap an sich
        void DrawMap(Minimap& map);

        /// Real size of the minimap (gets scaled with retained aspect ratio)
        unsigned short width_show, height_show;
        /// Abstand der Minimap vom Rand des Controls
        DrawPoint padding;
        unsigned short mapWidth_, mapHeight_;
};


#endif // !MapPreview_H_

