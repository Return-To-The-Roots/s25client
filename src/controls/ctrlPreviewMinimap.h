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
#ifndef PREVIEWMINIMAP_H_
#define PREVIEWMINIMAP_H_

#include "ctrlMinimap.h"
#include "PreviewMinimap.h"
#include "gameData/MaxPlayers.h"
#include "ogl/glArchivItem_Map.h"
class Window;

/// Übersichtskarte (MapPreview)
class ctrlPreviewMinimap : public ctrlMinimap
{
        /// Minimap
        PreviewMinimap minimap;
        /// Startpositionen der Spieler
        struct Player
        {
            Player();
            /// Map-Koordinaten der Startposition
            unsigned short x, y;
            /// Farbe
            unsigned color;
        } players[MAX_PLAYERS];

    public:

        ctrlPreviewMinimap( Window* parent,
                            const unsigned int id,
                            const unsigned short x,
                            const unsigned short y,
                            const unsigned short width,
                            const unsigned short height,
                            glArchivItem_Map* s2map);

        /// Zeichnet die MapPreview
        bool Draw_() override;

        /// Setzt die (Start-)Farbe eines Spielers bzw. löscht diesen (color = 0)
        void SetPlayerColor(const unsigned id, const unsigned color)
        {
            players[id].color = color;
        }

        void SetMap(const glArchivItem_Map* const s2map);
};


#endif // !MapPreview_H_

