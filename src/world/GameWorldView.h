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
#include <vector>

class GameWorldViewer;
struct RoadBuildState;
class TerrainRenderer;
class noBaseBuilding;

class IDebugNodePrinter{
public:
    virtual ~IDebugNodePrinter(){}
    /// Called when a node is going to be printed at displayPt
    /// Can e.g. print coordinates
    virtual void print(const MapPoint& pt, const Point<int>& displayPt) = 0;
};

struct ObjectBetweenLines;

class GameWorldView
{
    /// Currently selected point (where the mouse points to)
    MapPoint selPt;
    /// Offset to selected point
    Point<int> selPtOffset;

    /// Class for printing debug map data
    IDebugNodePrinter* debugNodePrinter;

    /// Show building quality icons
    bool show_bq;
    /// Show building names
    bool show_names;
    /// Show productivities
    bool show_productivity;

    /// Offset from world origin in screen units (not map units): "scroll position"
    Point<int> offset;
    /// Last scroll position (before jump)
    Point<int> lastOffset;
    /// First drawn map point (might be slightly outside map -> Wrapping)
    Point<int> firstPt;
    /// Last drawn map point
    Point<int> lastPt;

    GameWorldViewer& gwv;

    unsigned d_what;
    unsigned d_player;
    bool d_active;

    /// Top-Left position of the view (window)
    Point<int> pos;
    /// Size of the view
    unsigned width, height;

    /// How much the view is scaled (1=normal, >1=bigger, >0 && <1=smaller)
    float zoomFactor;

public:
    GameWorldView(GameWorldViewer& gwv, const Point<int>& pos, unsigned width, unsigned height);
    ~GameWorldView();

    GameWorldViewer& GetViewer() const { return gwv; }

    void SetPos(const Point<int>& newPos) { pos = newPos; }
    Point<int> GetPos() const { return pos; }

    void SetZoomFactor(float zoomFactor);

    /// Bauqualitäten anzeigen oder nicht
    void ToggleShowBQ() { show_bq = !show_bq; }
    /// Gebäudenamen zeigen oder nicht
    void ToggleShowNames() { show_names = !show_names; }
    /// Produktivität zeigen oder nicht
    void ToggleShowProductivity() { show_productivity = !show_productivity; };
    /// Schaltet Produktivitäten/Namen komplett aus oder an
    void ToggleShowNamesAndProductivity();

    void Draw(const RoadBuildState& rb, const bool draw_selected = false, const MapPoint selected = MapPoint::Invalid(), unsigned* water = NULL);

    /// Bewegt sich zu einer bestimmten Position in Pixeln auf der Karte
    void MoveTo(int x, int y, bool absolute = false);
    /// Zentriert den Bildschirm auf ein bestimmtes Map-Object
    void MoveToMapPt(const MapPoint pt);
    /// Springt zur letzten Position, bevor man "weggesprungen" ist
    void MoveToLastPosition();

    void MoveToX(int x, bool absolute = false) { MoveTo( (absolute ? 0 : offset.x) + x, offset.y, true); }
    void MoveToY(int y, bool absolute = false) { MoveTo( offset.x, (absolute ? 0 : offset.y) + y, true); }

    /// Set the debug node printer used. Max. 1 at a time. NULL for disabling
    void SetDebugNodePrinter(IDebugNodePrinter* newPrinter) { debugNodePrinter = newPrinter; }

    /// Gibt selektierten Punkt zurück
    MapPoint GetSelectedPt() const { return selPt; }

    /// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
    Point<int> GetFirstPt() const { return firstPt; }
    /// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
    Point<int> GetLastPt() const { return lastPt; }

    void Resize(unsigned width, unsigned height);

    void SetAIDebug(unsigned what, unsigned player, bool active)
    {
        d_what = what; d_player = player; d_active = active;
    }

private:
    void CalcFxLx();
    void DrawAIDebug(const MapPoint& pt, const Point<int>& curPos);
    void DrawBoundaryStone(const MapPoint& pt, const Point<int> pos, Visibility vis);
    void DrawObject(const MapPoint& pt, const Point<int>& curPos);
    void DrawConstructionAid(const MapPoint& pt, const Point<int>& curPos);
    void DrawFigures(const MapPoint& pt, const Point<int>&curPos, std::vector<ObjectBetweenLines>& between_lines);
    void DrawNameProductivityOverlay(const TerrainRenderer& terrainRenderer);
    void DrawProductivity(const noBaseBuilding& no, const Point<int>& curPos);
    void DrawGUI(const RoadBuildState& rb, const TerrainRenderer& terrainRenderer, const bool draw_selected, const MapPoint& selectedPt);

};

#endif // GameWorldView_h__
