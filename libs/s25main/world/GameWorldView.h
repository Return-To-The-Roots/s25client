// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Cheats.h"
#include "DrawPoint.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include <boost/signals2.hpp>
#include <optional>
#include <vector>

class GameWorldBase;
class GameWorldViewer;
class noBaseBuilding;
class SoundManager;
class TerrainRenderer;
struct RoadBuildState;

class IDrawNodeCallback
{
public:
    virtual ~IDrawNodeCallback() = default;
    /// Called when a node is going to be drawn at displayPt
    /// Can e.g. print coordinates
    virtual void onDraw(const MapPoint& pt, const DrawPoint& displayPt) = 0;
};

struct ObjectBetweenLines;

class GameWorldView
{
    struct RadiusPreview
    {
        MapPoint center;
        unsigned radius;
        std::vector<Position> outline;
    };

    const GameWorldViewer& gwv;

    /// Top-Left position of the view (window)
    Position origin_;
    /// Size of the view
    Extent size_;

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

    /// Optional radius preview (center position, radius) drawn as outline on the map
    std::optional<RadiusPreview> radiusPreview_;
    /// Cached addon status: whether building radius overlay is enabled
    bool isBuildingRadiusEnabled_;

    /// Offset from world origin in screen units (not map units): "scroll position"
    DrawPoint offset;
    /// Last scroll position (before jump)
    DrawPoint lastOffset;
    /// First drawn map point (might be slightly outside map -> Wrapping)
    DrawPoint firstPt;
    /// Last drawn map point
    DrawPoint lastPt;

    /// How much the view is scaled (1=normal, >1=bigger, >0 && <1=smaller)
    float zoomFactor_;
    float effectiveZoomFactor_; ///< DPI scale corrected zoom factor
    float targetZoomFactor_;
    float zoomSpeed_;

public:
    GameWorldView(const GameWorldViewer& gwv, const Position& pos, const Extent& size);

    const GameWorldViewer& getViewer() const { return gwv; }
    const GameWorldBase& getWorld() const;
    SoundManager& getSoundMgr();

    void setPos(const Position& newPos) { origin_ = newPos; }
    Position getPos() const { return origin_; }
    Extent getSize() const { return size_; }

    /// Set target zoom factor and start zooming if smoothTransition is true
    /// Returns actual zoom factor used, potentially clamped
    float setZoomFactor(float zoomFactor, bool smoothTransition = true);
    float getCurrentTargetZoomFactor() const;
    void setNextZoomFactor();

    // Converts a view coordinate to map position
    Position viewPosToMap(Position pos) const;

    /// Show or hide construction aid
    void toggleShowBQ();
    /// Show or hide building names
    void toggleShowNames();
    /// Show or hide productivity
    void toggleShowProductivity();
    /// Toggle names and productivity completely on or off
    void toggleShowNamesAndProductivity();

    /// Copy visibility of HUD elements from this view to another
    void copyHudSettingsTo(GameWorldView& other, bool copyBQ) const;

    void draw(const RoadBuildState& rb, MapPoint selected, bool drawMouse, unsigned* water = nullptr);

    /// Moves the map view by the given offset in pixels
    void moveBy(const DrawPoint& numPixels);
    /// Moves a position on the map in pixels
    void moveTo(const DrawPoint& newPos);
    /// Zentriert den Bildschirm auf ein bestimmtes Map-Object
    void moveToMapPt(MapPoint pt);
    /// Springt zur letzten Position, bevor man "weggesprungen" ist
    void moveToLastPosition();

    DrawPoint getOffset() const { return offset; }

    /// Get the radius in tiles for a building type (worker reach, territory influence, attack range, etc.)
    unsigned getBuildingRadius(BuildingType bld) const;

    /// Add a debug node printer
    void addDrawNodeCallback(IDrawNodeCallback* newCallback);
    void removeDrawNodeCallback(IDrawNodeCallback* callbackToRemove);

    /// Show a radius preview outline on the map
    void showRadiusPreview(MapPoint center, unsigned radius);
    void hideRadiusPreview() { radiusPreview_ = std::nullopt; }

    /// Update radius preview based on which map node is selected
    void updateRadiusPreviewForMousePos();

    /// Get radius preview currently shown
    [[nodiscard]] const std::optional<RadiusPreview>& getRadiusPreview() const noexcept { return radiusPreview_; }

    /// Snap a point to the nearest toroidal copy relative to a reference position, given the map size in pixels
    static DrawPoint snapToNearestCopy(DrawPoint pt, DrawPoint ref, Extent mapSize);

    /// Gibt selektierten Punkt zurück
    MapPoint getSelectedPt() const { return selPt; }

    /// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
    Position getFirstPt() const { return firstPt; }
    /// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
    Position getLastPt() const { return lastPt; }

    void resize(const Extent& newSize);

    /// Triggered when visibility of HUD elements changes
    boost::signals2::signal<void()> onHudSettingsChanged;

private:
    void calcFxLx();
    void drawBoundaryStone(const MapPoint& pt, DrawPoint pos, Visibility vis);
    void drawResource(const MapPoint& pt, DrawPoint curPos, Cheats::ResourceRevealMode resRevealMode);
    void drawObject(const MapPoint& pt, const DrawPoint& curPos) const;
    void drawConstructionAid(const MapPoint& pt, const DrawPoint& curPos);
    void drawFigures(const MapPoint& pt, const DrawPoint& curPos,
                     std::vector<ObjectBetweenLines>& objsBetweenRows) const;
    void drawMovingFiguresFromBelow(const TerrainRenderer& terrainRenderer, const DrawPoint& curPos,
                                    std::vector<ObjectBetweenLines>& objsBetweenRows);

    void drawNameProductivityOverlay(const TerrainRenderer& terrainRenderer);
    void drawProductivity(const noBaseBuilding& no, const DrawPoint& curPos);
    void drawGUI(const RoadBuildState& rb, const TerrainRenderer& terrainRenderer, const MapPoint& selectedPt,
                 bool drawMouse);

    /// Draw a radius outline ring
    void drawRadiusOutline();

    void saveIngameSettingsValues() const;
    void updateEffectiveZoomFactor();
};
