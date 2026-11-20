// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"
#include <vector>

namespace rttr::mapGenerator {

/// Themes of sceneries built around a special object
enum class SceneType
{
    /// Castle ruin on rocky ground between flowers and rocks
    CastleRuin,
    /// Ruin of a tower or guardhouse between flowers and trees
    RuinGrove,
    /// Rows of crosses on barren ground between dead trees and bones
    Graveyard,
    /// Single tent with a pile of wood on trampled ground
    Camp,
    /// Gate within a circle of standing stones
    StoneCircle
};
constexpr auto maxEnumValue(SceneType)
{
    return SceneType::StoneCircle;
}

/// Layouts arranging the surroundings of a special object
enum class SceneStyle
{
    /// Irregular patch with a cluster of rocks, a clump of trees and a drift of flowers in different directions
    Overgrown,
    /// Straight road between two rows of trees leading to a court which is enclosed by a hedge
    Avenue,
    /// Rocky hill behind the special object, a pond in front of it and flowers and trees on its sides
    Crag,
    /// Curved trail lined by remains which leads to the special object
    Trail
};
constexpr auto maxEnumValue(SceneStyle)
{
    return SceneStyle::Trail;
}

struct Scene
{
    SceneType type;
    SceneStyle style;
    MapPoint center;
};

/// Radius around the center of a scene which is covered by the scene
constexpr unsigned sceneRadius = 7;
/// Radius around the center of a scene which has to be buildable land without any objects
constexpr unsigned sceneCoreRadius = 4;

/// Indices of special objects (object type OT_Decoration)
constexpr uint8_t OI_Gate = 0x16;
constexpr uint8_t OI_OpenGate = 0x17;
constexpr uint8_t OI_Tent = 0x2C;
constexpr uint8_t OI_GuardhouseRuin = 0x2D;
constexpr uint8_t OI_TowerRuin = 0x2E;
constexpr uint8_t OI_CastleRuin = 0x2F;
constexpr uint8_t OI_Cross = 0x30;
constexpr uint8_t OI_PileOfWood = 0x32;

/// Checks whether the object is the centerpiece of a scene
bool isSpecialObject(uint8_t objectType, uint8_t objectInfo);

/// Desired number of scenes for the map size. Only a few are placed to keep them special.
unsigned getNumSpecialScenes(const MapExtent& size);

/**
 * Searches for up to `number` centers of scenes. The middle of a scene needs buildable land without objects and keeps
 * a distance to headquarters, harbors and other scenes. Does not modify the map.
 */
std::vector<MapPoint> findSceneSites(const Map& map, RandomUtility& rnd, unsigned number);

/**
 * Creates a scene around the center: the type defines the objects and the ground, the style their arrangement.
 * Only buildable land without objects is changed. The nodes of the scene are reserved to keep them free from trees and
 * stone piles.
 */
void createScene(Map& map, RandomUtility& rnd, const MapPoint& center, SceneType type, SceneStyle style);

/**
 * Places a few scenes of mixed types and random styles on the map.
 * Must be called once textures, headquarters and harbors are in place but before any other objects are added.
 */
std::vector<Scene> addSpecialScenes(Map& map, RandomUtility& rnd);

} // namespace rttr::mapGenerator
