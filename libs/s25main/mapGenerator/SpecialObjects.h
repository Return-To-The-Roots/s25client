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
bool IsSpecialObject(uint8_t objectType, uint8_t objectInfo);

/**
 * Computes the number of special scenes for a map. Only a few scenes are placed to keep them special.
 *
 * @param size size of the map
 *
 * @returns the desired number of scenes which grows with the size of the map.
 */
unsigned GetNumSpecialScenes(const MapExtent& size);

/**
 * Searches for places for special scenes. The middle of a scene requires buildable land without any objects and keeps
 * a distance to headquarters, harbors and other scenes.
 *
 * @param map map to search for places (is not modified)
 * @param rnd random number generator
 * @param number desired number of scenes
 *
 * @returns the center of each scene - less than desired if the map doesn't provide enough suitable places.
 */
std::vector<MapPoint> FindSceneSites(const Map& map, RandomUtility& rnd, unsigned number);

/**
 * Creates a scene around the specified center. The theme defines the special object, the objects around it and the
 * ground while the style defines how they are arranged. The ground blends into the surrounding land using intermediate
 * textures and hills rise smoothly. Only buildable land without objects is changed: water, mountains and other
 * landscapes in the outer part of a scene are left untouched. The nodes of the scene are reserved to keep them free
 * from random trees and stone piles.
 *
 * @param map reference to the map to place the scene on (manipulates textures, objects and z-values of the map)
 * @param rnd random number generator
 * @param center center of the scene which gets the special object
 * @param type theme of the scene
 * @param style layout of the scene
 */
void CreateScene(Map& map, RandomUtility& rnd, const MapPoint& center, SceneType type, SceneStyle style);

/**
 * Places a few special scenes on the map. The types of the scenes are mixed up and each scene gets a random
 * style. This function has to be called once
 * textures, headquarters and harbors are in place but before any other objects are added.
 *
 * @param map reference to the map to place the scenes on (manipulates textures, objects and z-values of the map)
 * @param rnd random number generator
 *
 * @returns the type and center of each new scene.
 */
std::vector<Scene> AddSpecialScenes(Map& map, RandomUtility& rnd);

} // namespace rttr::mapGenerator
