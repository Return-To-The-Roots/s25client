// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/SpecialObjects.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "helpers/mathFuncs.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/FreeLand.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/TextureHelper.h"
#include "gameTypes/Direction.h"
#include "libsiedler2/enumTypes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace rttr::mapGenerator {

namespace {
    /// Minimum distance of the center of a scene to headquarters and harbors. Objects close to them are removed.
    constexpr unsigned minPlayerDistance = sceneRadius + 6;
    /// Radius of the ground in the middle of a scene which is leveled before hills are added
    constexpr unsigned levelRadius = 3;
    constexpr double pi = 3.14159265358979323846;
    /// Texture indices which are the same for all landscapes
    constexpr uint8_t flowerTextureId = 0x0F; // meadow covered by flowers
    constexpr uint8_t rockTextureId = 0x22;   // flat rock

    using Objects = std::vector<uint8_t>;
    const Objects mushrooms{0x00, 0x01, 0x22};
    const Objects stones{0x02, 0x03, 0x04};
    const Objects pebbles{0x25, 0x26, 0x27};
    const Objects deadTrees{OI_DeadTreeTrunk, OI_DeadTree, 0x1F, 0x20};
    const Objects bones{OI_Bone1, OI_Bone2};
    const Objects flowers{0x09};
    const Objects bushes{0x0A, 0x10, 0x11, 0x12, 0x28, 0x29, 0x2A};
    const Objects shrubs{0x0E, 0x0F, 0x13, 0x14};
    const Objects standingStones{0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E};
    const Objects cacti{0x0C, 0x0D};
    /// Bushes and shrubs of all kinds for hedges and thickets
    const Objects undergrowth{0x0A, 0x10, 0x11, 0x12, 0x28, 0x29, 0x2A, 0x0E, 0x0F, 0x13, 0x14};
    /// Mostly flowers mixed with small plants and mushrooms for colorful beds and drifts
    const Objects plants{0x09, 0x09, 0x09, 0x09, 0x09, 0x2A, 0x29, 0x0E, 0x13, 0x14, 0x00, 0x01, 0x22};
    /// Landscape index of the winter world which has no cacti
    constexpr uint8_t winterLandscapeId = 2;

    /// Ground of a patch - either blended into the surrounding land or painted on top of it
    enum class Ground
    {
        None,
        /// Driest land which looks like trampled ground
        Dry,
        /// Meadow covered by flowers
        Lush,
        /// Flat rock with a rim of mountain meadow
        Rock
    };

    /// Textures painted on top of the land in ascending priority
    enum class Overlay : uint8_t
    {
        None,
        RockRim,
        Rock,
        Sand,
        Water
    };

    /// Objects and ground making up the character of a type of scene
    struct Theme
    {
        /// Special object in the center
        Objects main;
        /// Larger objects used for clusters and along paths
        Objects props;
        /// Small objects scattered on the ground
        Objects litter;
        /// Ground right around the special object
        Ground core;
        /// Ground of the wider surroundings
        Ground surround;
        bool hill;
        bool flowers;
        bool granite;
        bool deadVegetation;
    };

    Objects join(Objects lhs, const Objects& rhs)
    {
        lhs.insert(lhs.end(), rhs.begin(), rhs.end());
        return lhs;
    }

    Theme makeTheme(SceneType type, bool isWinter)
    {
        // Dry ground gets cacti unless it's frozen
        const Objects dryPlants = isWinter ? pebbles : join(pebbles, cacti);

        switch(type)
        {
            case SceneType::CastleRuin:
                return {{OI_CastleRuin}, stones, pebbles, Ground::Rock, Ground::Lush, true, true, true, false};
            case SceneType::RuinGrove:
                return {{OI_TowerRuin, OI_GuardhouseRuin},
                        stones,
                        mushrooms,
                        Ground::Lush,
                        Ground::Lush,
                        true,
                        true,
                        false,
                        false};
            case SceneType::Graveyard:
                return {{OI_Cross}, {OI_Cross}, join(bones, mushrooms), Ground::Dry, Ground::Dry, false, false,
                        false,      true};
            case SceneType::Camp:
                return {{OI_Tent}, dryPlants, dryPlants, Ground::Dry, Ground::None, false, true, false, false};
            case SceneType::StoneCircle:
                return {{OI_Gate, OI_OpenGate},
                        standingStones,
                        mushrooms,
                        Ground::Rock,
                        Ground::Lush,
                        true,
                        true,
                        false,
                        false};
        }
        throw std::invalid_argument("invalid type of scene");
    }

    /// Area of irregular shape around a node
    struct Blob
    {
        MapPoint center;
        double radius;
        /// Phases of the waves deforming the outline - a circle if amplitude is zero
        double amplitude, phase1, phase2;
    };

    /// Helper which collects the ground, the hills and the objects of a scene
    class SceneBuilder
    {
        Map& map_;
        RandomUtility& rnd_;
        const MapPoint center_;
        const Theme theme_;
        /// Two kinds of trees - the first one dominates
        const Tree tree_, otherTree_;
        /// Buildable land textures from dry to lush used for smooth transitions
        std::vector<DescIdx<TerrainDesc>> ramp_;
        /// Nodes with a desired index into the ramp in the order they were requested
        std::vector<std::pair<MapPoint, int>> ground_;
        /// Requests for the ground around the special object which take precedence over all others
        std::vector<std::pair<MapPoint, int>> coreGround_;
        NodeMapBase<Overlay> overlay_;
        /// Nodes which have to stay free from objects
        std::set<MapPoint, MapPointLess> blocked_;
        /// Nodes of the scene which may be changed: buildable land without objects. Only the middle of a scene is
        /// known to be free. Water, mountains and other landscapes further out are left untouched and cut the scene.
        std::set<MapPoint, MapPointLess> free_;

        static unsigned mod6(int dir) { return static_cast<unsigned>((dir % 6 + 6) % 6); }

        std::pair<double, double> planar(const MapPoint& from, const MapPoint& to) const
        {
            const auto delta = [](int value, int size) {
                if(value > size / 2)
                    return value - size;
                return value < -size / 2 ? value + size : value;
            };
            const int dx = delta(static_cast<int>(to.x) - from.x, map_.size.x);
            const int dy = delta(static_cast<int>(to.y) - from.y, map_.size.y);
            // Odd rows are shifted to the right by half a node
            return {dx + 0.5 * ((to.y & 1) - (from.y & 1)), dy * 0.866};
        }

        /// Index into the ramp of the land texture of the node
        int rampIndex(const MapPoint& pt) const
        {
            const auto it = std::find(ramp_.begin(), ramp_.end(), map_.getTextures()[pt].rsu);
            return it != ramp_.end() ? static_cast<int>(it - ramp_.begin()) : static_cast<int>(ramp_.size() / 2);
        }

        int target(Ground ground) const { return ground == Ground::Dry ? 0 : static_cast<int>(ramp_.size()) - 1; }

        DescIdx<TerrainDesc> overlayTexture(Overlay overlay) const
        {
            const auto isRock = [](const TerrainDesc& t) {
                return t.is(ETerrain::Buildable) && t.s2Id == rockTextureId;
            };
            switch(overlay)
            {
                case Overlay::RockRim: return map_.textureMap.find(isBuildableMountain);
                case Overlay::Rock: return map_.textureMap.find(isRock);
                case Overlay::Sand: return map_.textureMap.find(isCoastTerrain);
                default: return map_.textureMap.find(isShipableWater);
            }
        }

    public:
        SceneBuilder(Map& map, RandomUtility& rnd, const MapPoint& center, SceneType type)
            : map_(map), rnd_(rnd), center_(center),
              theme_(makeTheme(type, map.textureMap.getLandscapeId() == winterLandscapeId)),
              tree_(rnd.randomItem(createTrees(map.textureMap))),
              otherTree_(rnd.randomItem(createTrees(map.textureMap)))
        {
            const auto sceneArea = map.z.getPointsInRadiusWithCenter(center, sceneRadius);
            for(const MapPoint& pt : sceneArea)
            {
                if(map.textureMap.all(pt, isBuildableLand) && map.objectTypes[pt] == libsiedler2::OT_Empty
                   && !helpers::contains(map.barren, pt))
                    free_.insert(pt);
            }
            map.reserved.insert(sceneArea.begin(), sceneArea.end());
            overlay_.resize(map.size, Overlay::None);

            // Flowers are the most lush texture. The index makes the order independent from the sort algorithm.
            ramp_ = map.textureMap.findAll(isBuildableLand);
            map.textureMap.sort(ramp_, [](const TerrainDesc& terrain) {
                return std::make_tuple(terrain.humidity, terrain.s2Id == flowerTextureId, terrain.s2Id);
            });

            // Level the middle of the scene and reduce the slopes towards the surrounding land
            const int level = map.z[center];
            for(const MapPoint& pt : free_)
            {
                const int tolerance = std::max(0, static_cast<int>(distance(pt)) - static_cast<int>(levelRadius));
                map.z[pt] =
                  static_cast<uint8_t>(helpers::clamp<int>(map.z[pt], std::max<int>(level - tolerance, minZ()),
                                                           std::min<int>(level + tolerance, map.height.maximum)));
            }
        }

        const MapPoint& center() const { return center_; }
        const Theme& getTheme() const { return theme_; }
        RandomUtility& rnd() { return rnd_; }
        int minZ() const { return map_.height.minimum + 1; }
        unsigned distance(const MapPoint& pt) const { return map_.z.calcDistance(center_, pt); }
        unsigned distance(const MapPoint& from, const MapPoint& to) const { return map_.z.calcDistance(from, to); }
        bool isPond(const MapPoint& pt) const { return overlay_[pt] == Overlay::Water; }
        bool isFree(const MapPoint& pt) const { return helpers::contains(free_, pt); }

        /// Moves from the node into the direction which is counted clockwise starting in the west
        MapPoint step(MapPoint pt, int dir, unsigned steps = 1) const
        {
            for(unsigned i = 0; i < steps; i++)
                pt = map_.z.getNeighbour(pt, Direction(mod6(dir)));
            return pt;
        }

        /// All nodes with exactly the specified distance to the center in circular order starting in the west
        std::vector<MapPoint> ring(unsigned radius) const
        {
            std::vector<MapPoint> result;
            MapPoint pt = step(center_, 0, radius);
            for(int dir = 2; dir < 8; dir++)
            {
                for(unsigned i = 0; i < radius; i++)
                {
                    result.push_back(pt);
                    pt = step(pt, dir);
                }
            }
            return result;
        }

        std::vector<MapPoint> area(const MapPoint& pt, unsigned radius) const
        {
            auto result = map_.z.getPointsInRadiusWithCenter(pt, radius);
            helpers::eraseIf(result, [this](const MapPoint& p) { return !isFree(p); });
            return result;
        }

        double angle(const MapPoint& from, const MapPoint& to) const
        {
            const auto [x, y] = planar(from, to);
            return std::atan2(y, x);
        }

        double randomAngle() { return rnd_.randomDouble(-pi, pi); }

        Blob makeBlob(const MapPoint& center, double radius, bool irregular = true)
        {
            return Blob{center, radius, irregular ? 1. : 0., randomAngle(), randomAngle()};
        }

        /// Relative position within a blob: 0 at the center, 1 at the outline and above 1 outside
        double eval(const Blob& blob, const MapPoint& pt) const
        {
            const double theta = angle(blob.center, pt);
            const double deformation =
              0.22 * std::sin(2 * theta + blob.phase1) + 0.13 * std::sin(3 * theta + blob.phase2);
            return distance(blob.center, pt) / (blob.radius * (1. + blob.amplitude * deformation));
        }

        std::vector<MapPoint> nodes(const Blob& blob, double limit = 1.) const
        {
            auto result = area(blob.center, static_cast<unsigned>(blob.radius * 1.4) + 1);
            helpers::eraseIf(result, [this, &blob, limit](const MapPoint& pt) { return eval(blob, pt) >= limit; });
            return result;
        }

        /// Requests the ground for the blob. Later requests take precedence where grounds meet - except for the
        /// ground around the special object which always wins.
        void blend(const Blob& blob, Ground ground, bool isCore = false)
        {
            if(ground == Ground::None)
                return;
            if(ground == Ground::Rock)
            {
                // Rock never touches the land directly but is enclosed by a rim
                // Only the ground of the special object itself reaches the middle of the scene
                const auto isAllowed = [this, isCore](const MapPoint& pt) { return isCore || distance(pt) > 1; };
                for(const MapPoint& pt : nodes(blob, 0.75))
                {
                    if(!isAllowed(pt))
                        continue;
                    setOverlay(pt, Overlay::Rock);
                    for(const MapPoint& neighbor : area(pt, 1))
                    {
                        if(isAllowed(neighbor))
                            setOverlay(neighbor, Overlay::RockRim);
                    }
                }
                return;
            }

            // Only the inner part gets the ground - the transition to the surrounding land is added in the end
            for(const MapPoint& pt : nodes(blob, 0.75))
                (isCore ? coreGround_ : ground_).emplace_back(pt, target(ground));
        }

        void setOverlay(const MapPoint& pt, Overlay overlay)
        {
            if(isFree(pt) && distance(pt) < sceneRadius - 1 && overlay > overlay_[pt])
                overlay_[pt] = overlay;
        }

        /// Raises (or lowers) a smooth hill around the node
        void bump(const MapPoint& top, int height, double radius)
        {
            for(const MapPoint& pt : area(top, static_cast<unsigned>(radius)))
            {
                const double position = distance(top, pt) / radius;
                if(position >= 1.)
                    continue;
                const int elevation = static_cast<int>(std::lround(height * 0.5 * (1. + std::cos(pi * position))));
                map_.z[pt] = static_cast<uint8_t>(std::clamp<int>(map_.z[pt] + elevation, minZ(), map_.height.maximum));
            }
        }

        void block(const MapPoint& pt) { blocked_.insert(pt); }

        bool canPlace(const MapPoint& pt) const
        {
            return isFree(pt) && map_.objectTypes[pt] == libsiedler2::OT_Empty && !helpers::contains(blocked_, pt)
                   && !isPond(pt);
        }

        bool chance(unsigned percentage) { return percentage >= 100 || rnd_.byChance(percentage); }

        void place(const MapPoint& pt, const Objects& objects)
        {
            if(!canPlace(pt))
                return;
            map_.objectTypes[pt] = OT_Decoration;
            map_.objectInfos[pt] = rnd_.randomItem(objects);
        }

        void placeTree(const MapPoint& pt)
        {
            if(!canPlace(pt))
                return;
            const Tree& tree = rnd_.byChance(75) ? tree_ : otherTree_;
            map_.objectTypes[pt] = tree.type;
            map_.objectInfos[pt] = tree.index + rnd_.randomValue(0, 7);
        }

        void placeGranite(const MapPoint& pt)
        {
            if(!canPlace(pt))
                return;
            map_.objectTypes[pt] = rnd_.byChance(50) ? libsiedler2::OT_Stone1 : libsiedler2::OT_Stone2;
            map_.objectInfos[pt] = static_cast<uint8_t>(rnd_.randomValue(2, 6));
        }

        /// Trees mixed with some undergrowth or dead trees for gloomy themes
        void placeVegetation(const MapPoint& pt)
        {
            if(theme_.deadVegetation)
                place(pt, rnd_.byChance(80) ? deadTrees : mushrooms);
            else if(rnd_.byChance(85))
                placeTree(pt);
            else
                place(pt, undergrowth);
        }

        /// Trees of a single kind for rows or dead trees for gloomy themes
        void placeRowTree(const MapPoint& pt)
        {
            if(theme_.deadVegetation)
                place(pt, deadTrees);
            else if(canPlace(pt))
            {
                map_.objectTypes[pt] = tree_.type;
                map_.objectInfos[pt] = tree_.index + rnd_.randomValue(0, 7);
            }
        }

        /// Large rocks or the props of the theme
        void placeRock(const MapPoint& pt)
        {
            if(theme_.granite && rnd_.byChance(60))
                placeGranite(pt);
            else
                place(pt, theme_.granite ? stones : theme_.props);
        }

        /// Flowers mixed with other small plants or the litter of the theme
        void placeGroundCover(const MapPoint& pt) { place(pt, theme_.flowers ? plants : theme_.litter); }

        /// Calls the function by chance for the nodes of a sector around the center
        template<class T_Place>
        void forSector(double direction, double halfWidth, unsigned minDistance, unsigned maxDistance,
                       unsigned percentage, T_Place&& action)
        {
            for(const MapPoint& pt : area(center_, maxDistance))
            {
                if(distance(pt) < minDistance)
                    continue;
                double difference = std::fabs(angle(center_, pt) - direction);
                difference = std::min(difference, 2 * pi - difference);
                if(difference <= halfWidth && chance(percentage))
                    action(pt);
            }
        }

        /// Calls the function by chance for the nodes of a blob
        template<class T_Place>
        void forBlob(const Blob& blob, double limit, unsigned percentage, T_Place&& action)
        {
            for(const MapPoint& pt : nodes(blob, limit))
            {
                if(chance(percentage))
                    action(pt);
            }
        }

        /// Places the special object and the objects which make the type of scene recognizable in any style
        void placeCore(SceneType type)
        {
            map_.objectTypes[center_] = OT_Decoration;
            map_.objectInfos[center_] = rnd_.randomItem(theme_.main);
            block(center_);
            const auto innerRing = ring(1);
            const auto outerRing = ring(2);

            switch(type)
            {
                case SceneType::CastleRuin:
                    // The ruin covers some of its neighbors
                    for(const int dir : {0, 1, 2})
                        block(step(center_, dir));
                    break;
                case SceneType::Graveyard:
                    // Three rows of graves
                    for(const unsigned index : {0u, 3u})
                        place(innerRing[index], theme_.main);
                    for(const unsigned index : {2u, 3u, 4u, 8u, 9u, 10u})
                        place(outerRing[index], theme_.main);
                    break;
                case SceneType::Camp: place(innerRing[rnd_.index(innerRing.size())], {OI_PileOfWood}); break;
                case SceneType::StoneCircle:
                    for(unsigned i = 0; i < outerRing.size(); i += 2)
                        place(outerRing[i], standingStones);
                    break;
                default: break;
            }
        }

        /// Applies the collected ground to the map and makes sure the special object stands on even ground
        void finish()
        {
            flattenForCastleBuilding(map_.z, center_);

            // Ponds are flat
            const auto sceneArea = map_.z.getPointsInRadiusWithCenter(center_, sceneRadius);
            int waterLevel = map_.height.maximum;
            for(const MapPoint& pt : sceneArea)
            {
                if(isPond(pt))
                    waterLevel = std::min<int>(waterLevel, map_.z[pt]);
            }
            for(const MapPoint& pt : sceneArea)
            {
                if(isPond(pt))
                    map_.z[pt] = static_cast<uint8_t>(waterLevel);
            }

            ground_.insert(ground_.end(), coreGround_.begin(), coreGround_.end());
            paintGround(sceneArea);
            for(const Overlay overlay : {Overlay::RockRim, Overlay::Rock, Overlay::Sand, Overlay::Water})
            {
                for(const MapPoint& pt : sceneArea)
                {
                    if(overlay_[pt] == overlay)
                        map_.textureMap.set(pt, overlayTexture(overlay));
                }
            }
        }

    private:
        /// Paints the requested grounds with transitions: neighboring nodes differ by at most one step of the ramp.
        /// Sand counts as one step below the driest land. On conflicts the surrounding land, shores and later requests
        /// win.
        void paintGround(const std::vector<MapPoint>& sceneArea)
        {
            struct Anchor
            {
                MapPoint pt;
                int index;
                bool isBorder = false;
            };
            std::vector<Anchor> anchors;
            // All kinds of meadow look alike next to dry land: the land around the scene only asks for the
            // transitions up to the first of them. Otherwise shores close to the border would get none at all.
            const int firstMeadow = std::min<int>(2, static_cast<int>(ramp_.size()) - 1);
            const auto limit = [this, &anchors, firstMeadow](const MapPoint& pt, int index) {
                int lower = -100;
                int upper = 100;
                for(const Anchor& anchor : anchors)
                {
                    const int dist = static_cast<int>(distance(anchor.pt, pt));
                    lower =
                      std::max(lower, (anchor.isBorder ? std::min(anchor.index, firstMeadow) : anchor.index) - dist);
                    upper = std::min(upper, anchor.index + dist);
                }
                // Anchors may contradict each other if the land around the scene has no transitions itself
                return lower > upper ? (lower + upper) / 2 : std::clamp(index, lower, upper);
            };

            std::set<MapPoint, MapPointLess> visited;
            std::vector<Anchor> painted;
            const auto addAnchor = [&](const MapPoint& pt, int index, bool paint) {
                if(!visited.insert(pt).second)
                    return;
                const Anchor anchor{pt, limit(pt, index)};
                if(paint && anchor.index != rampIndex(pt))
                    painted.push_back(anchor);
                anchors.push_back(anchor);
            };

            // The land right outside the scene is not painted
            for(const MapPoint& pt : ring(sceneRadius + 1))
            {
                visited.insert(pt);
                anchors.push_back(Anchor{pt, rampIndex(pt), true});
            }
            for(const MapPoint& pt : sceneArea)
            {
                if(overlay_[pt] == Overlay::Sand || overlay_[pt] == Overlay::Water)
                    addAnchor(pt, -1, false);
            }
            for(auto it = ground_.rbegin(); it != ground_.rend(); ++it)
            {
                if(isFree(it->first))
                    addAnchor(it->first, it->second, true);
            }
            // All other nodes keep their texture unless a transition is required
            for(const MapPoint& pt : sceneArea)
            {
                if(visited.count(pt) || !isFree(pt))
                    continue;
                const int index = limit(pt, rampIndex(pt));
                if(index != rampIndex(pt))
                    painted.push_back(Anchor{pt, index});
            }

            // Neighboring nodes share triangles: the texture which differs most from the land is painted last. Thus
            // patches keep their size and each transition gets a row of triangles.
            const auto change = [this](const Anchor& anchor) { return std::abs(anchor.index - rampIndex(anchor.pt)); };
            std::stable_sort(painted.begin(), painted.end(),
                             [&change](const Anchor& lhs, const Anchor& rhs) { return change(lhs) < change(rhs); });
            for(const Anchor& anchor : painted)
            {
                const int index = std::clamp<int>(anchor.index, 0, static_cast<int>(ramp_.size()) - 1);
                map_.textureMap.set(anchor.pt, ramp_[index]);
            }
        }
    };

    /// Irregular patch with a cluster of rocks, a clump of trees and a drift of flowers in different directions
    void createOvergrown(SceneBuilder& scene)
    {
        const Theme& theme = scene.getTheme();
        const MapPoint center = scene.center();

        scene.blend(scene.makeBlob(center, 5.2), theme.surround);
        scene.blend(scene.makeBlob(center, theme.core == Ground::Rock ? 2.0 : 2.8), theme.core, true);
        if(theme.hill)
            scene.bump(center, 2, 5.);

        // A path leads through the gap between the rocks and the trees
        const int pathDir = static_cast<int>(scene.rnd().randomValue(0, 5));
        for(unsigned i = 1; i < sceneRadius; i++)
        {
            const MapPoint pt = scene.step(scene.step(center, pathDir, i), pathDir + 1, i / 3);
            scene.block(pt);
            if(theme.surround != Ground::Dry && i > 1)
                scene.blend(scene.makeBlob(pt, 1.7, false), Ground::Dry);
        }

        const double pathAngle = scene.angle(center, scene.step(center, pathDir, 3));
        const double rockAngle = pathAngle + scene.rnd().randomDouble(1.2, 1.9);
        const double treeAngle = pathAngle - scene.rnd().randomDouble(1.2, 1.9);
        const auto wrap = [](double angle) { return std::atan2(std::sin(angle), std::cos(angle)); };

        scene.forSector(wrap(rockAngle), 0.65, 2, 5, 55, [&scene](const MapPoint& pt) { scene.placeRock(pt); });
        scene.forSector(wrap(treeAngle), 0.8, 3, sceneRadius, 70,
                        [&scene](const MapPoint& pt) { scene.placeVegetation(pt); });
        scene.forSector(wrap(pathAngle + pi), 0.9, 2, 5, 50,
                        [&scene](const MapPoint& pt) { scene.placeGroundCover(pt); });
        scene.forSector(0., pi, 2, 6, 7, [&scene](const MapPoint& pt) { scene.place(pt, undergrowth); });
        scene.forSector(0., pi, 1, 4, 10, [&scene, &theme](const MapPoint& pt) { scene.place(pt, theme.litter); });
    }

    /// Straight road between two rows of trees leading to a court which is enclosed by a hedge
    void createAvenue(SceneBuilder& scene)
    {
        const Theme& theme = scene.getTheme();
        const MapPoint center = scene.center();
        const int dir = static_cast<int>(scene.rnd().randomValue(0, 5));
        const Ground paving = theme.core == Ground::Lush ? Ground::Dry : theme.core;

        scene.blend(scene.makeBlob(center, 4.6), theme.surround);
        scene.blend(scene.makeBlob(center, paving == Ground::Rock ? 2.9 : 3.1, false), paving, true);
        scene.bump(center, theme.hill ? 2 : 1, 5.);

        std::vector<MapPoint> road;
        for(unsigned i = 1; i <= sceneRadius; i++)
        {
            road.push_back(scene.step(center, dir, i));
            scene.block(road.back());
            if(paving == Ground::Rock)
                scene.setOverlay(road.back(), Overlay::RockRim);
            else
                scene.blend(scene.makeBlob(road.back(), 1.1, false), Ground::Dry);
        }
        const auto nearRoad = [&scene, &road](const MapPoint& pt) {
            return helpers::containsIf(road, [&](const MapPoint& node) { return scene.distance(node, pt) <= 1; });
        };

        // Trees on both sides of the road outside the court
        for(unsigned i = 3; i < road.size(); i++)
        {
            scene.placeRowTree(scene.step(road[i], dir + 2));
            scene.placeRowTree(scene.step(road[i], dir + 4));
        }

        // Beds within the court and a hedge around it
        for(const MapPoint& pt : scene.ring(2))
        {
            if(!nearRoad(pt))
                scene.placeGroundCover(pt);
        }
        const auto hedge = scene.ring(3);
        for(unsigned i = 0; i < hedge.size(); i++)
        {
            if(nearRoad(hedge[i]))
                continue;
            if(theme.deadVegetation)
            {
                if(i % 2 == 0)
                    scene.place(hedge[i], deadTrees);
            } else
                scene.place(hedge[i], undergrowth);
        }
        // Rocks or props mark the corners
        const auto outerRing = scene.ring(4);
        for(unsigned i = 0; i < outerRing.size(); i += 4)
        {
            if(!nearRoad(outerRing[i]))
                scene.placeRock(outerRing[i]);
        }
    }

    /// Rocky hill behind the special object, a pond in front of it and flowers and trees on its sides
    void createCrag(SceneBuilder& scene)
    {
        const Theme& theme = scene.getTheme();
        const MapPoint center = scene.center();
        const int dir = static_cast<int>(scene.rnd().randomValue(0, 5));
        const int side = scene.rnd().byChance(50) ? 2 : 4;
        const MapPoint crag = scene.step(center, dir, 3);
        const MapPoint pond = scene.step(center, dir + 3, 3);

        scene.blend(scene.makeBlob(center, 3.2), theme.surround);
        scene.blend(scene.makeBlob(center, theme.core == Ground::Rock ? 1.6 : 2.2), theme.core, true);

        const Blob cragBlob = scene.makeBlob(crag, 2.9);
        scene.blend(cragBlob, Ground::Rock);
        scene.bump(crag, 4, 4.4);
        scene.forBlob(cragBlob, 0.95, 55, [&scene](const MapPoint& pt) {
            if(scene.rnd().byChance(65))
                scene.placeGranite(pt);
            else
                scene.place(pt, stones);
        });
        scene.forBlob(scene.makeBlob(scene.step(center, dir, 6), 2.2), 1., 60,
                      [&scene](const MapPoint& pt) { scene.placeVegetation(pt); });

        // Pond with a sandy shore and reed at its far side. It stays close to the middle of the scene to leave space
        // for the transition from sand to the surrounding land.
        scene.bump(pond, -1, 3.);
        std::vector<MapPoint> water{pond, scene.step(pond, dir + 3)};
        for(const MapPoint& pt : scene.area(pond, 1))
        {
            if(scene.distance(pt) >= 3 && scene.rnd().byChance(60) && !helpers::contains(water, pt))
                water.push_back(pt);
        }
        for(const MapPoint& pt : water)
        {
            scene.setOverlay(pt, Overlay::Water);
            scene.block(pt);
        }
        for(const MapPoint& pt : water)
        {
            for(const MapPoint& neighbor : scene.area(pt, 1))
            {
                if(scene.isPond(neighbor))
                    continue;
                scene.setOverlay(neighbor, Overlay::Sand);
                if(scene.distance(neighbor) > scene.distance(pond) && scene.rnd().byChance(50))
                    scene.place(neighbor, shrubs);
            }
        }

        const Blob drift = scene.makeBlob(scene.step(center, dir + side, 4), 2.5);
        scene.blend(drift, Ground::Lush);
        scene.forBlob(drift, 0.85, 55, [&scene, &theme](const MapPoint& pt) {
            if(theme.flowers)
                scene.place(pt, plants);
            else
                scene.place(pt, theme.props);
        });
        scene.forBlob(scene.makeBlob(scene.step(center, dir - side, 5), 2.1), 1., 65,
                      [&scene](const MapPoint& pt) { scene.placeVegetation(pt); });
        scene.forSector(0., pi, 1, 3, 12, [&scene, &theme](const MapPoint& pt) { scene.place(pt, theme.litter); });
    }

    /// Curved trail lined by remains which leads to the special object
    void createTrail(SceneBuilder& scene)
    {
        const Theme& theme = scene.getTheme();
        const MapPoint center = scene.center();
        const int dir = static_cast<int>(scene.rnd().randomValue(0, 5));
        const int turn = scene.rnd().byChance(50) ? 1 : -1;

        scene.blend(scene.makeBlob(center, 3.4), theme.surround);
        scene.blend(scene.makeBlob(center, theme.core == Ground::Rock ? 1.9 : 2.6), theme.core, true);
        if(theme.hill)
            scene.bump(center, 2, 4.5);

        MapPoint pt = center;
        std::vector<std::pair<MapPoint, int>> trail;
        for(const int bend : {0, 0, 1, 1, 1, 2, 2})
        {
            pt = scene.step(pt, dir + turn * bend);
            trail.emplace_back(pt, dir + turn * bend);
            scene.block(pt);
        }
        for(const auto& node : trail)
            scene.blend(scene.makeBlob(node.first, 1.7, false), Ground::Dry);

        // Remains alternate on both sides of the trail
        for(unsigned i = 1; i < trail.size(); i++)
        {
            const int left = trail[i].second + 2;
            const int right = trail[i].second + 4;
            scene.place(scene.step(trail[i].first, i % 2 ? left : right), theme.props);
            if(scene.rnd().byChance(45))
                scene.place(scene.step(trail[i].first, i % 2 ? right : left), theme.litter);
        }
        scene.placeRock(scene.step(trail.back().first, trail.back().second + 1));

        // Clumps of trees behind the special object and inside of the bend
        scene.forBlob(scene.makeBlob(scene.step(center, dir + 3, 4), 2.4), 1., 65,
                      [&scene](const MapPoint& p) { scene.placeVegetation(p); });
        scene.forBlob(scene.makeBlob(scene.step(trail[3].first, trail[3].second + turn * 2, 3), 1.9), 1., 60,
                      [&scene](const MapPoint& p) { scene.placeVegetation(p); });
        scene.forSector(0., pi, 1, 3, 30, [&scene](const MapPoint& p) { scene.placeGroundCover(p); });
        scene.forSector(0., pi, 2, 6, 8, [&scene](const MapPoint& p) { scene.place(p, undergrowth); });
    }
} // namespace

bool isSpecialObject(uint8_t objectType, uint8_t objectInfo)
{
    if(objectType != OT_Decoration)
        return false;
    return objectInfo == OI_Gate || objectInfo == OI_OpenGate || objectInfo == OI_Tent
           || objectInfo == OI_GuardhouseRuin || objectInfo == OI_TowerRuin || objectInfo == OI_CastleRuin
           || objectInfo == OI_Cross;
}

unsigned getNumSpecialScenes(const MapExtent& size)
{
    return helpers::clamp((size.x + size.y) / 96u, 1u, 6u);
}

std::vector<MapPoint> findSceneSites(const Map& map, RandomUtility& rnd, unsigned number)
{
    const auto isUnsuitable = [&map](const MapPoint& pt) {
        return !map.textureMap.all(pt, isBuildableLand) || map.objectTypes[pt] != libsiedler2::OT_Empty
               || helpers::contains(map.barren, pt) || helpers::contains(map.reserved, pt);
    };
    const auto obstacleDistance = distancesTo(map.size, isUnsuitable);

    std::vector<MapPoint> players = map.hqPositions;
    for(const Triangle& harbor : map.harbors)
        players.push_back(harbor.position);
    const auto playerDistance = distancesTo(players, map.size);

    // Only the middle of a scene requires free land - one node more leaves a gap to the surroundings. Obstacles
    // further out cut the scene which allows scenes on maps with little free land.
    auto candidates = selectPoints(
      [&](const MapPoint& pt) {
          return obstacleDistance[pt] > sceneCoreRadius + 1 && playerDistance[pt] >= minPlayerDistance;
      },
      map.size);

    const unsigned minSceneDistance = std::max<unsigned>(4 * sceneRadius, (map.size.x + map.size.y) / 16u);
    std::vector<MapPoint> sites;

    while(sites.size() < number && !candidates.empty())
    {
        const MapPoint site = rnd.randomItem(candidates);
        sites.push_back(site);
        helpers::eraseIf(candidates, [&map, &site, minSceneDistance](const MapPoint& pt) {
            return map.z.calcDistance(site, pt) < minSceneDistance;
        });
    }

    return sites;
}

void createScene(Map& map, RandomUtility& rnd, const MapPoint& center, SceneType type, SceneStyle style)
{
    SceneBuilder scene(map, rnd, center, type);
    scene.placeCore(type);

    switch(style)
    {
        case SceneStyle::Overgrown: createOvergrown(scene); break;
        case SceneStyle::Avenue: createAvenue(scene); break;
        case SceneStyle::Crag: createCrag(scene); break;
        case SceneStyle::Trail: createTrail(scene); break;
    }

    scene.finish();
}

std::vector<Scene> addSpecialScenes(Map& map, RandomUtility& rnd)
{
    // Each type is used once before any type is repeated
    std::vector<SceneType> types;
    for(const SceneType type : helpers::EnumRange<SceneType>())
        types.push_back(type);
    for(size_t i = types.size(); i > 1; i--)
        std::swap(types[i - 1], types[rnd.index(i)]);

    std::vector<Scene> scenes;
    for(const MapPoint& site : findSceneSites(map, rnd, getNumSpecialScenes(map.size)))
    {
        const SceneType type = types[scenes.size() % types.size()];
        const auto style =
          static_cast<SceneStyle>(rnd.randomValue(0u, static_cast<unsigned>(maxEnumValue(SceneStyle{}))));
        createScene(map, rnd, site, type, style);
        scenes.push_back(Scene{type, style, site});
    }

    return scenes;
}

} // namespace rttr::mapGenerator
