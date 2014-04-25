// $Id: ctrlMinimap.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef CTRL_MINIMAP_H_
#define CTRL_MINIMAP_H_

#include "ctrlRectangle.h"
#include "MapConsts.h"
#include "glArchivItem_Bitmap_Direct.h"

class Minimap;

/// Übersichtskarte (MapPreview)
class ctrlMinimap : public Window
{
    protected:
        /// Größe der Anzeige der Minimap
        unsigned short width_show, height_show;
        /// Abstand der Minimap vom Rand des Controls
        unsigned short padding_x, padding_y;
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
        inline unsigned short GetLeft() const { return padding_x + (width - width_show - 2 * padding_x) / 2; }
        inline unsigned short GetTop() const { return padding_y + (height - height_show - 2 * padding_y) / 2; }
        inline unsigned short GetRight() const { return GetLeft() + width_show + padding_x; }
        inline unsigned short GetBottom() const { return GetTop() + height_show + padding_y; }

        /// Größe ändern
        virtual void Resize_(unsigned short width, unsigned short heigth);
        void SetDisplaySize(const unsigned short width, const unsigned short height,
                            const unsigned short map_width, const unsigned short map_height);

        /// Liefert für einen gegebenen Map-Punkt die Pixel-Koordinaten relativ zur Bounding-Box
        inline unsigned short CalcMapCoordX(const unsigned short x) const
        { return GetLeft() + width_show * x / map_width; }
        inline unsigned short CalcMapCoordY(const unsigned short y) const
        { return GetTop() + height_show * y / map_height; }

        /// Verkleinert Minimap soweit es geht (entfernt Bounding-Box) in Y-Richtung und gibt neue Höhe zurück
        void RemoveBoundingBox(const unsigned short width_min, const unsigned short height_min);

    protected:

        /// Zeichnet die Minimap an sich
        void DrawMap(Minimap& map);

        ///// Berechnet X-Koordinaten der rechten Seiten
        //unsigned short GetLeft() const { return x+minimap.GetLeft(); }
        ///// Berechnet Y-Koordinate der unteren Kante
        //unsigned short GetBottom() const { return y+minimap.GetBottom(); }

    protected:

        unsigned short map_width;
        unsigned short map_height;
};


#endif // !MapPreview_H_

