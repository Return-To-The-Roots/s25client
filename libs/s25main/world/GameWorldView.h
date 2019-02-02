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

#ifndef GameWorldView_h__
#define GameWorldView_h__

#include "DrawPoint.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include <vector>

class GameWorldViewer;
class GameWorldBase;
struct RoadBuildState;
class TerrainRenderer;
class noBaseBuilding;

class IDrawNodeCallback
{
public:
    virtual ~IDrawNodeCallback() {}
    /// Called when a node is going to be drawn at displayPt
    /// Can e.g. print coordinates
    virtual void onDraw(const MapPoint& pt, const DrawPoint& displayPt) = 0;
};

struct ObjectBetweenLines;

class GameWorldView
{
    /// Currently selected point (where the mouse points to)
    MapPoint selPt;
    /// Offset to selected point
    Position selPtOffset;

    /// Callbacks called when node is printed
    std::vector<IDrawNodeCallback*> drawNodeCallbacks;

    /// Show building quality icons
    bool show_bq;
    /// Show building names
    bool show_names;
    /// Show productivities
    bool show_productivity;

    /// Offset from world origin in screen units (not map units): "scroll position"
    DrawPoint offset;
    /// Last scroll position (before jump)
    DrawPoint lastOffset;
    /// First drawn map point (might be slightly outside map -> Wrapping)
    DrawPoint firstPt;
    /// Last drawn map point
    DrawPoint lastPt;

    const GameWorldViewer& gwv;

    /// Top-Left position of the view (window)
    Position origin_;
    /// Size of the view
    Extent size_;

    /// How much the view is scaled (1=normal, >1=bigger, >0 && <1=smaller)
    float zoomFactor_;
    float targetZoomFactor_;
    float zoomSpeed_;

public:
    GameWorldView(const GameWorldViewer& gwv, const Position& pos, const Extent& size);
    ~GameWorldView();

    const GameWorldViewer& GetViewer() const { return gwv; }
    const GameWorldBase& GetWorld() const;

    void SetPos(const Position& newPos) { origin_ = newPos; }
    Position GetPos() const { return origin_; }
    Extent GetSize() const { return size_; }

    void SetZoomFactor(float zoomFactor, bool smoothTransition = true);
    float GetCurrentTargetZoomFactor() const;
    void SetNextZoomFactor();

    /// Bauqualitäten anzeigen oder nicht
    void ToggleShowBQ() { show_bq = !show_bq; }
    /// Gebäudenamen zeigen oder nicht
    void ToggleShowNames() { show_names = !show_names; }
    /// Produktivität zeigen oder nicht
    void ToggleShowProductivity() { show_productivity = !show_productivity; };
    /// Schaltet Produktivitäten/Namen komplett aus oder an
    void ToggleShowNamesAndProductivity();

    void Draw(const RoadBuildState& rb, const MapPoint selected, bool drawMouse, unsigned* water = nullptr);

    /// Bewegt sich zu einer bestimmten Position in Pixeln auf der Karte
    void MoveTo(int x, int y, bool absolute = false);
    void MoveTo(const DrawPoint& newPos, bool absolute = false);
    /// Zentriert den Bildschirm auf ein bestimmtes Map-Object
    void MoveToMapPt(const MapPoint pt);
    /// Springt zur letzten Position, bevor man "weggesprungen" ist
    void MoveToLastPosition();

    void MoveToX(int x, bool absolute = false) { MoveTo((absolute ? 0 : offset.x) + x, offset.y, true); }
    void MoveToY(int y, bool absolute = false) { MoveTo(offset.x, (absolute ? 0 : offset.y) + y, true); }
    DrawPoint GetOffset() const { return offset; }

    /// Add a debug node printer
    void AddDrawNodeCallback(IDrawNodeCallback* newCallback);
    void RemoveDrawNodeCallback(IDrawNodeCallback* callbackToRemove);

    /// Gibt selektierten Punkt zurück
    MapPoint GetSelectedPt() const { return selPt; }

    /// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
    Position GetFirstPt() const { return firstPt; }
    /// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
    Position GetLastPt() const { return lastPt; }

    void Resize(const Extent& newSize);

private:
    void CalcFxLx();
    void DrawBoundaryStone(const MapPoint& pt, const DrawPoint pos, Visibility vis);
    void DrawObject(const MapPoint& pt, const DrawPoint& curPos);
    void DrawConstructionAid(const MapPoint& pt, const DrawPoint& curPos);
    void DrawFigures(const MapPoint& pt, const DrawPoint& curPos, std::vector<ObjectBetweenLines>& between_lines);
    void DrawMovingFiguresFromBelow(const TerrainRenderer& terrainRenderer, const DrawPoint& curPos,
                                    std::vector<ObjectBetweenLines>& between_lines);

    void DrawNameProductivityOverlay(const TerrainRenderer& terrainRenderer);
    void DrawProductivity(const noBaseBuilding& no, const DrawPoint& curPos);
    void DrawGUI(const RoadBuildState& rb, const TerrainRenderer& terrainRenderer, const MapPoint& selectedPt, bool drawMouse);
};

#endif // GameWorldView_h__
