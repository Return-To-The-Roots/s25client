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

#ifndef GameWorldView_h__
#define GameWorldView_h__

#include "gameTypes/MapTypes.h"
#include "Point.h"

class GameWorldViewer;
struct RoadsBuilding;

class GameWorldView
{
    /// Selektierter Punkt
    MapPoint selPt;
    Point<int> selO;

    /// Koordinaten auf der Map anzeigen (zum Debuggen)?
    bool show_coordinates;

    bool show_bq;    ///< Bauqualitäten-Anzeigen ein oder aus
    bool show_names; ///< Gebäudenamen-Anzeigen ein oder aus
    bool show_productivity; ///< Produktivität-Anzeigen ein oder aus

    /// Scrolling-Zeug
    Point<int> offset;
    /// Letzte Scrollposition, an der man war, bevor man weggesprungen ist
    Point<int> lastOffset;
    /// Erster gezeichneter Map-Punkt
    Point<int> firstPt;
    /// Letzter gezeichneter Map-Punkt
    Point<int> lastPt;

    GameWorldViewer* gwv;

protected:
    unsigned d_what;
    unsigned d_player;
    bool d_active;

    MapPoint pos;
    unsigned short width, height;

public:
    bool terrain_rerender;
    unsigned int terrain_list;
    Point<int> terrainLastOffset;
    unsigned int terrain_last_global_animation;
    unsigned int terrain_last_water;

    GameWorldView(const MapPoint pt, unsigned short width, unsigned short height);
    ~GameWorldView();

    GameWorldViewer& GetGameWorldViewer() const {return *gwv;}
    void SetGameWorldViewer(GameWorldViewer* viewer);


    inline void SetPos(MapPoint newPos) { pos = newPos; }
    inline MapPoint GetPos() const {return pos;}

    /// Bauqualitäten anzeigen oder nicht
    inline void ShowBQ() { show_bq = !show_bq; }
    /// Gebäudenamen zeigen oder nicht
    inline void ShowNames() { show_names = !show_names; }
    /// Produktivität zeigen oder nicht
    inline void ShowProductivity() { show_productivity = !show_productivity; };
    /// Schaltet Produktivitäten/Namen komplett aus oder an
    void ShowNamesAndProductivity();

    void Draw(const unsigned char player, unsigned* water, const bool draw_selected, const MapPoint selected, const RoadsBuilding& rb);
    /*
        void PrepareRendering(const unsigned char player, const bool draw_selected, const MapCoord selected_x, const MapCoord selected_y,const RoadsBuilding& rb);
        void Render();
    */

    /// Bewegt sich zu einer bestimmten Position in Pixeln auf der Karte
    void MoveTo(int x, int y, bool absolute = false);
    /// Zentriert den Bildschirm auf ein bestimmtes Map-Object
    void MoveToMapObject(const MapPoint pt);
    /// Springt zur letzten Position, bevor man "weggesprungen" ist
    void MoveToLastPosition();

    inline void MoveToX(int x, bool absolute = false) { MoveTo( (absolute ? 0 : offset.x) + x, offset.y, true); }
    inline void MoveToY(int y, bool absolute = false) { MoveTo( offset.x, (absolute ? 0 : offset.y) + y, true); }

    void CalcFxLx();

    /// Koordinatenanzeige ein/aus
    inline void ShowCoordinates() { show_coordinates = !show_coordinates; }

    /// Gibt selektierten Punkt zurück
    inline MapCoord GetSelX() const { return selPt.x; }
    inline MapCoord GetSelY() const { return selPt.y; }
    inline MapPoint GetSel() const { return selPt; }

    inline Point<int> GetSelo() const { return selO; }

    /// Gibt Scrolling-Offset zurück
    inline int GetXOffset() const { return offset.x - pos.x; }
    inline int GetYOffset() const { return offset.y - pos.y; }
    inline Point<int> GetOffset() const { return offset - Point<int>(pos); }
    /// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
    inline Point<int> GetFirstPt() const { return firstPt; }
    /// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
    inline Point<int> GetLastPt() const { return lastPt; }

    void DrawBoundaryStone(const int x, const int y, const MapPoint t, const Point<int> pos, Visibility vis);

    void Resize(unsigned short width, unsigned short height);

    void SetAIDebug(unsigned what, unsigned player, bool active)
    {
        d_what = what; d_player = player; d_active = active;
    }
};

#endif // GameWorldView_h__