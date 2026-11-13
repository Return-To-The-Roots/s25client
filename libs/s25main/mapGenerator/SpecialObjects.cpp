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

    Objects Join(Objects lhs, const Objects& rhs)
    {
        lhs.insert(lhs.end(), rhs.begin(), rhs.end());
        return lhs;
    }

    Theme MakeTheme(SceneType type, bool isWinter)
    {
        // Dry ground gets cacti unless it's frozen
        const Objects dryPlants = isWinter ? pebbles : Join(pebbles, cacti);

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
                return {{OI_Cross}, {OI_Cross}, Join(bones, mushrooms), Ground::Dry, Ground::Dry, false, false,
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

        static unsigned Mod6(int dir) { return static_cast<unsigned>((dir % 6 + 6) % 6); }

        std::pair<double, double> Planar(const MapPoint& from, const MapPoint& to) const
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
        int RampIndex(const MapPoint& pt) const
        {
            const auto it = std::find(ramp_.begin(), ramp_.end(), map_.getTextures()[pt].rsu);
            return it != ramp_.end() ? static_cast<int>(it - ramp_.begin()) : static_cast<int>(ramp_.size() / 2);
        }

        int Target(Ground ground) const { return ground == Ground::Dry ? 0 : static_cast<int>(ramp_.size()) - 1; }

        DescIdx<TerrainDesc> OverlayTexture(Overlay overlay) const
        {
            const auto isRock = [](const TerrainDesc& t) {
                return t.Is(ETerrain::Buildable) && t.s2Id == rockTextureId;
            };
            switch(overlay)
            {
                case Overlay::RockRim: return map_.textureMap.Find(IsBuildableMountain);
                case Overlay::Rock: return map_.textureMap.Find(isRock);
                case Overlay::Sand: return map_.textureMap.Find(IsCoastTerrain);
                default: return map_.textureMap.Find(IsShipableWater);
            }
        }

    public:
        SceneBuilder(Map& map, RandomUtility& rnd, const MapPoint& center, SceneType type)
            : map_(map), rnd_(rnd), center_(center),
              theme_(MakeTheme(type, map.textureMap.GetLandscapeId() == winterLandscapeId)),
              tree_(rnd.RandomItem(CreateTrees(map.textureMap))),
              otherTree_(rnd.RandomItem(CreateTrees(map.textureMap)))
        {
            const auto area = map.z.GetPointsInRadiusWithCenter(center, sceneRadius);
            for(const MapPoint& pt : area)
            {
                if(map.textureMap.All(pt, IsBuildableLand) && map.objectTypes[pt] == libsiedler2::OT_Empty
                   && !helpers::contains(map.barren, pt))
                    free_.insert(pt);
            }
            map.reserved.insert(area.begin(), area.end());
            overlay_.Resize(map.size, Overlay::None);

            // Flowers are the most lush texture. The index makes the order independent from the sort algorithm.
            ramp_ = map.textureMap.FindAll(IsBuildableLand);
            map.textureMap.Sort(ramp_, [](const TerrainDesc& terrain) {
                return std::make_tuple(terrain.humidity, terrain.s2Id == flowerTextureId, terrain.s2Id);
            });

            // Level the middle of the scene and reduce the slopes towards the surrounding land
            const int level = map.z[center];
            for(const MapPoint& pt : free_)
            {
                const int tolerance = std::max(0, static_cast<int>(Distance(pt)) - static_cast<int>(levelRadius));
                map.z[pt] =
                  static_cast<uint8_t>(helpers::clamp<int>(map.z[pt], std::max<int>(level - tolerance, MinZ()),
                                                           std::min<int>(level + tolerance, map.height.maximum)));
            }
        }

        const MapPoint& Center() const { return center_; }
        const Theme& GetTheme() const { return theme_; }
        RandomUtility& Rnd() { return rnd_; }
        int MinZ() const { return map_.height.minimum + 1; }
        unsigned Distance(const MapPoint& pt) const { return map_.z.CalcDistance(center_, pt); }
        unsigned Distance(const MapPoint& from, const MapPoint& to) const { return map_.z.CalcDistance(from, to); }
        bool IsWater(const MapPoint& pt) const { return overlay_[pt] == Overlay::Water; }
        bool IsFree(const MapPoint& pt) const { return helpers::contains(free_, pt); }

        /// Moves from the node into the direction which is counted clockwise starting in the west
        MapPoint Step(MapPoint pt, int dir, unsigned steps = 1) const
        {
            for(unsigned i = 0; i < steps; i++)
                pt = map_.z.GetNeighbour(pt, Direction(Mod6(dir)));
            return pt;
        }

        /// All nodes with exactly the specified distance to the center in circular order starting in the west
        std::vector<MapPoint> Ring(unsigned radius) const
        {
            std::vector<MapPoint> ring;
            MapPoint pt = Step(center_, 0, radius);
            for(int dir = 2; dir < 8; dir++)
            {
                for(unsigned i = 0; i < radius; i++)
                {
                    ring.push_back(pt);
                    pt = Step(pt, dir);
                }
            }
            return ring;
        }

        std::vector<MapPoint> Area(const MapPoint& pt, unsigned radius) const
        {
            auto area = map_.z.GetPointsInRadiusWithCenter(pt, radius);
            helpers::erase_if(area, [this](const MapPoint& p) { return !IsFree(p); });
            return area;
        }

        double Angle(const MapPoint& from, const MapPoint& to) const
        {
            const auto planar = Planar(from, to);
            return std::atan2(planar.second, planar.first);
        }

        double RandomAngle() { return rnd_.RandomDouble(-pi, pi); }

        Blob MakeBlob(const MapPoint& center, double radius, bool irregular = true)
        {
            return Blob{center, radius, irregular ? 1. : 0., RandomAngle(), RandomAngle()};
        }

        /// Relative position within a blob: 0 at the center, 1 at the outline and above 1 outside
        double Eval(const Blob& blob, const MapPoint& pt) const
        {
            const double angle = Angle(blob.center, pt);
            const double deformation =
              0.22 * std::sin(2 * angle + blob.phase1) + 0.13 * std::sin(3 * angle + blob.phase2);
            return Distance(blob.center, pt) / (blob.radius * (1. + blob.amplitude * deformation));
        }

        std::vector<MapPoint> Nodes(const Blob& blob, double limit = 1.) const
        {
            auto nodes = Area(blob.center, static_cast<unsigned>(blob.radius * 1.4) + 1);
            helpers::erase_if(nodes, [this, &blob, limit](const MapPoint& pt) { return Eval(blob, pt) >= limit; });
            return nodes;
        }

        /// Requests the ground for the blob. Later requests take precedence where grounds meet - except for the
        /// ground around the special object which always wins.
        void Blend(const Blob& blob, Ground ground, bool isCore = false)
        {
            if(ground == Ground::None)
                return;
            if(ground == Ground::Rock)
            {
                // Rock never touches the land directly but is enclosed by a rim
                // Only the ground of the special object itself reaches the middle of the scene
                const auto isAllowed = [this, isCore](const MapPoint& pt) { return isCore || Distance(pt) > 1; };
                for(const MapPoint& pt : Nodes(blob, 0.75))
                {
                    if(!isAllowed(pt))
                        continue;
                    SetOverlay(pt, Overlay::Rock);
                    for(const MapPoint& neighbor : Area(pt, 1))
                    {
                        if(isAllowed(neighbor))
                            SetOverlay(neighbor, Overlay::RockRim);
                    }
                }
                return;
            }

            // Only the inner part gets the ground - the transition to the surrounding land is added in the end
            for(const MapPoint& pt : Nodes(blob, 0.75))
                (isCore ? coreGround_ : ground_).emplace_back(pt, Target(ground));
        }

        void SetOverlay(const MapPoint& pt, Overlay overlay)
        {
            if(IsFree(pt) && Distance(pt) < sceneRadius - 1 && overlay > overlay_[pt])
                overlay_[pt] = overlay;
        }

        /// Raises (or lowers) a smooth hill around the node
        void Bump(const MapPoint& top, int height, double radius)
        {
            for(const MapPoint& pt : Area(top, static_cast<unsigned>(radius)))
            {
                const double position = Distance(top, pt) / radius;
                if(position >= 1.)
                    continue;
                const int elevation = static_cast<int>(std::lround(height * 0.5 * (1. + std::cos(pi * position))));
                map_.z[pt] = static_cast<uint8_t>(std::clamp<int>(map_.z[pt] + elevation, MinZ(), map_.height.maximum));
            }
        }

        void Block(const MapPoint& pt) { blocked_.insert(pt); }

        bool CanPlace(const MapPoint& pt) const
        {
            return IsFree(pt) && map_.objectTypes[pt] == libsiedler2::OT_Empty && !helpers::contains(blocked_, pt)
                   && !IsWater(pt);
        }

        bool Chance(unsigned chance) { return chance >= 100 || rnd_.ByChance(chance); }

        void Place(const MapPoint& pt, const Objects& objects)
        {
            if(!CanPlace(pt))
                return;
            map_.objectTypes[pt] = OT_Decoration;
            map_.objectInfos[pt] = rnd_.RandomItem(objects);
        }

        void PlaceTree(const MapPoint& pt)
        {
            if(!CanPlace(pt))
                return;
            const Tree& tree = rnd_.ByChance(75) ? tree_ : otherTree_;
            map_.objectTypes[pt] = tree.type;
            map_.objectInfos[pt] = tree.index + rnd_.RandomValue(0, 7);
        }

        void PlaceGranite(const MapPoint& pt)
        {
            if(!CanPlace(pt))
                return;
            map_.objectTypes[pt] = rnd_.ByChance(50) ? libsiedler2::OT_Stone1 : libsiedler2::OT_Stone2;
            map_.objectInfos[pt] = static_cast<uint8_t>(rnd_.RandomValue(2, 6));
        }

        /// Trees mixed with some undergrowth or dead trees for gloomy themes
        void PlaceVegetation(const MapPoint& pt)
        {
            if(theme_.deadVegetation)
                Place(pt, rnd_.ByChance(80) ? deadTrees : mushrooms);
            else if(rnd_.ByChance(85))
                PlaceTree(pt);
            else
                Place(pt, undergrowth);
        }

        /// Trees of a single kind for rows or dead trees for gloomy themes
        void PlaceRowTree(const MapPoint& pt)
        {
            if(theme_.deadVegetation)
                Place(pt, deadTrees);
            else if(CanPlace(pt))
            {
                map_.objectTypes[pt] = tree_.type;
                map_.objectInfos[pt] = tree_.index + rnd_.RandomValue(0, 7);
            }
        }

        /// Large rocks or the props of the theme
        void PlaceRock(const MapPoint& pt)
        {
            if(theme_.granite && rnd_.ByChance(60))
                PlaceGranite(pt);
            else
                Place(pt, theme_.granite ? stones : theme_.props);
        }

        /// Flowers mixed with other small plants or the litter of the theme
        void PlaceGroundCover(const MapPoint& pt) { Place(pt, theme_.flowers ? plants : theme_.litter); }

        /// Calls the function by chance for the nodes of a sector around the center
        template<class T_Place>
        void ForSector(double angle, double halfWidth, unsigned minDistance, unsigned maxDistance, unsigned chance,
                       T_Place&& place)
        {
            for(const MapPoint& pt : Area(center_, maxDistance))
            {
                if(Distance(pt) < minDistance)
                    continue;
                double difference = std::fabs(Angle(center_, pt) - angle);
                difference = std::min(difference, 2 * pi - difference);
                if(difference <= halfWidth && Chance(chance))
                    place(pt);
            }
        }

        /// Calls the function by chance for the nodes of a blob
        template<class T_Place>
        void ForBlob(const Blob& blob, double limit, unsigned chance, T_Place&& place)
        {
            for(const MapPoint& pt : Nodes(blob, limit))
            {
                if(Chance(chance))
                    place(pt);
            }
        }

        /// Places the special object and the objects which make the type of scene recognizable in any style
        void PlaceCore(SceneType type)
        {
            map_.objectTypes[center_] = OT_Decoration;
            map_.objectInfos[center_] = rnd_.RandomItem(theme_.main);
            Block(center_);
            const auto innerRing = Ring(1);
            const auto ring = Ring(2);

            switch(type)
            {
                case SceneType::CastleRuin:
                    // The ruin covers some of its neighbors
                    for(const int dir : {0, 1, 2})
                        Block(Step(center_, dir));
                    break;
                case SceneType::Graveyard:
                    // Three rows of graves
                    for(const unsigned index : {0u, 3u})
                        Place(innerRing[index], theme_.main);
                    for(const unsigned index : {2u, 3u, 4u, 8u, 9u, 10u})
                        Place(ring[index], theme_.main);
                    break;
                case SceneType::Camp: Place(innerRing[rnd_.Index(innerRing.size())], {OI_PileOfWood}); break;
                case SceneType::StoneCircle:
                    for(unsigned i = 0; i < ring.size(); i += 2)
                        Place(ring[i], standingStones);
                    break;
                default: break;
            }
        }

        /// Applies the collected ground to the map and makes sure the special object stands on even ground
        void Finish()
        {
            FlattenForCastleBuilding(map_.z, center_);

            // Ponds are flat
            const auto area = map_.z.GetPointsInRadiusWithCenter(center_, sceneRadius);
            int waterLevel = map_.height.maximum;
            for(const MapPoint& pt : area)
            {
                if(IsWater(pt))
                    waterLevel = std::min<int>(waterLevel, map_.z[pt]);
            }
            for(const MapPoint& pt : area)
            {
                if(IsWater(pt))
                    map_.z[pt] = static_cast<uint8_t>(waterLevel);
            }

            ground_.insert(ground_.end(), coreGround_.begin(), coreGround_.end());
            PaintGround(area);
            for(const Overlay overlay : {Overlay::RockRim, Overlay::Rock, Overlay::Sand, Overlay::Water})
            {
                for(const MapPoint& pt : area)
                {
                    if(overlay_[pt] == overlay)
                        map_.textureMap.Set(pt, OverlayTexture(overlay));
                }
            }
        }

    private:
        /**
         * Paints the requested grounds and adds transitions: the textures of neighboring nodes never differ by more
         * than one step of the ramp from dry to lush land. A patch of dry ground within meadow is therefore surrounded
         * by one ring for each texture in between - just like coasts. Sand counts as one step below the driest land.
         * The land around the scene, the shores and later requests keep their texture where requests are in conflict.
         */
        void PaintGround(const std::vector<MapPoint>& area)
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
                    const int distance = static_cast<int>(Distance(anchor.pt, pt));
                    lower = std::max(lower,
                                     (anchor.isBorder ? std::min(anchor.index, firstMeadow) : anchor.index) - distance);
                    upper = std::min(upper, anchor.index + distance);
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
                if(paint && anchor.index != RampIndex(pt))
                    painted.push_back(anchor);
                anchors.push_back(anchor);
            };

            // The land right outside the scene is not painted
            for(const MapPoint& pt : Ring(sceneRadius + 1))
            {
                visited.insert(pt);
                anchors.push_back(Anchor{pt, RampIndex(pt), true});
            }
            for(const MapPoint& pt : area)
            {
                if(overlay_[pt] == Overlay::Sand || overlay_[pt] == Overlay::Water)
                    addAnchor(pt, -1, false);
            }
            for(auto it = ground_.rbegin(); it != ground_.rend(); ++it)
            {
                if(IsFree(it->first))
                    addAnchor(it->first, it->second, true);
            }
            // All other nodes keep their texture unless a transition is required
            for(const MapPoint& pt : area)
            {
                if(visited.count(pt) || !IsFree(pt))
                    continue;
                const int index = limit(pt, RampIndex(pt));
                if(index != RampIndex(pt))
                    painted.push_back(Anchor{pt, index});
            }

            // Neighboring nodes share triangles: the texture which differs most from the land is painted last. Thus
            // patches keep their size and each transition gets a row of triangles.
            const auto change = [this](const Anchor& anchor) { return std::abs(anchor.index - RampIndex(anchor.pt)); };
            std::stable_sort(painted.begin(), painted.end(),
                             [&change](const Anchor& lhs, const Anchor& rhs) { return change(lhs) < change(rhs); });
            for(const Anchor& anchor : painted)
            {
                const int index = std::clamp<int>(anchor.index, 0, static_cast<int>(ramp_.size()) - 1);
                map_.textureMap.Set(anchor.pt, ramp_[index]);
            }
        }
    };

    /// Irregular patch with a cluster of rocks, a clump of trees and a drift of flowers in different directions
    void CreateOvergrown(SceneBuilder& scene)
    {
        const Theme& theme = scene.GetTheme();
        const MapPoint center = scene.Center();

        scene.Blend(scene.MakeBlob(center, 5.2), theme.surround);
        scene.Blend(scene.MakeBlob(center, theme.core == Ground::Rock ? 2.0 : 2.8), theme.core, true);
        if(theme.hill)
            scene.Bump(center, 2, 5.);

        // A path leads through the gap between the rocks and the trees
        const int pathDir = static_cast<int>(scene.Rnd().RandomValue(0, 5));
        for(unsigned i = 1; i < sceneRadius; i++)
        {
            const MapPoint pt = scene.Step(scene.Step(center, pathDir, i), pathDir + 1, i / 3);
            scene.Block(pt);
            if(theme.surround != Ground::Dry && i > 1)
                scene.Blend(scene.MakeBlob(pt, 1.7, false), Ground::Dry);
        }

        const double pathAngle = scene.Angle(center, scene.Step(center, pathDir, 3));
        const double rockAngle = pathAngle + scene.Rnd().RandomDouble(1.2, 1.9);
        const double treeAngle = pathAngle - scene.Rnd().RandomDouble(1.2, 1.9);
        const auto wrap = [](double angle) { return std::atan2(std::sin(angle), std::cos(angle)); };

        scene.ForSector(wrap(rockAngle), 0.65, 2, 5, 55, [&scene](const MapPoint& pt) { scene.PlaceRock(pt); });
        scene.ForSector(wrap(treeAngle), 0.8, 3, sceneRadius, 70,
                        [&scene](const MapPoint& pt) { scene.PlaceVegetation(pt); });
        scene.ForSector(wrap(pathAngle + pi), 0.9, 2, 5, 50,
                        [&scene](const MapPoint& pt) { scene.PlaceGroundCover(pt); });
        scene.ForSector(0., pi, 2, 6, 7, [&scene](const MapPoint& pt) { scene.Place(pt, undergrowth); });
        scene.ForSector(0., pi, 1, 4, 10, [&scene, &theme](const MapPoint& pt) { scene.Place(pt, theme.litter); });
    }

    /// Straight road between two rows of trees leading to a court which is enclosed by a hedge
    void CreateAvenue(SceneBuilder& scene)
    {
        const Theme& theme = scene.GetTheme();
        const MapPoint center = scene.Center();
        const int dir = static_cast<int>(scene.Rnd().RandomValue(0, 5));
        const Ground paving = theme.core == Ground::Lush ? Ground::Dry : theme.core;

        scene.Blend(scene.MakeBlob(center, 4.6), theme.surround);
        scene.Blend(scene.MakeBlob(center, paving == Ground::Rock ? 2.9 : 3.1, false), paving, true);
        scene.Bump(center, theme.hill ? 2 : 1, 5.);

        std::vector<MapPoint> road;
        for(unsigned i = 1; i <= sceneRadius; i++)
        {
            road.push_back(scene.Step(center, dir, i));
            scene.Block(road.back());
            if(paving == Ground::Rock)
                scene.SetOverlay(road.back(), Overlay::RockRim);
            else
                scene.Blend(scene.MakeBlob(road.back(), 1.1, false), Ground::Dry);
        }
        const auto nearRoad = [&scene, &road](const MapPoint& pt) {
            return helpers::contains_if(road, [&](const MapPoint& node) { return scene.Distance(node, pt) <= 1; });
        };

        // Trees on both sides of the road outside the court
        for(unsigned i = 3; i < road.size(); i++)
        {
            scene.PlaceRowTree(scene.Step(road[i], dir + 2));
            scene.PlaceRowTree(scene.Step(road[i], dir + 4));
        }

        // Beds within the court and a hedge around it
        for(const MapPoint& pt : scene.Ring(2))
        {
            if(!nearRoad(pt))
                scene.PlaceGroundCover(pt);
        }
        const auto hedge = scene.Ring(3);
        for(unsigned i = 0; i < hedge.size(); i++)
        {
            if(nearRoad(hedge[i]))
                continue;
            if(theme.deadVegetation)
            {
                if(i % 2 == 0)
                    scene.Place(hedge[i], deadTrees);
            } else
                scene.Place(hedge[i], undergrowth);
        }
        // Rocks or props mark the corners
        const auto outerRing = scene.Ring(4);
        for(unsigned i = 0; i < outerRing.size(); i += 4)
        {
            if(!nearRoad(outerRing[i]))
                scene.PlaceRock(outerRing[i]);
        }
    }

    /// Rocky hill behind the special object, a pond in front of it and flowers and trees on its sides
    void CreateCrag(SceneBuilder& scene)
    {
        const Theme& theme = scene.GetTheme();
        const MapPoint center = scene.Center();
        const int dir = static_cast<int>(scene.Rnd().RandomValue(0, 5));
        const int side = scene.Rnd().ByChance(50) ? 2 : 4;
        const MapPoint crag = scene.Step(center, dir, 3);
        const MapPoint pond = scene.Step(center, dir + 3, 3);

        scene.Blend(scene.MakeBlob(center, 3.2), theme.surround);
        scene.Blend(scene.MakeBlob(center, theme.core == Ground::Rock ? 1.6 : 2.2), theme.core, true);

        const Blob cragBlob = scene.MakeBlob(crag, 2.9);
        scene.Blend(cragBlob, Ground::Rock);
        scene.Bump(crag, 4, 4.4);
        scene.ForBlob(cragBlob, 0.95, 55, [&scene](const MapPoint& pt) {
            if(scene.Rnd().ByChance(65))
                scene.PlaceGranite(pt);
            else
                scene.Place(pt, stones);
        });
        scene.ForBlob(scene.MakeBlob(scene.Step(center, dir, 6), 2.2), 1., 60,
                      [&scene](const MapPoint& pt) { scene.PlaceVegetation(pt); });

        // Pond with a sandy shore and reed at its far side. It stays close to the middle of the scene to leave space
        // for the transition from sand to the surrounding land.
        scene.Bump(pond, -1, 3.);
        std::vector<MapPoint> water{pond, scene.Step(pond, dir + 3)};
        for(const MapPoint& pt : scene.Area(pond, 1))
        {
            if(scene.Distance(pt) >= 3 && scene.Rnd().ByChance(60) && !helpers::contains(water, pt))
                water.push_back(pt);
        }
        for(const MapPoint& pt : water)
        {
            scene.SetOverlay(pt, Overlay::Water);
            scene.Block(pt);
        }
        for(const MapPoint& pt : water)
        {
            for(const MapPoint& neighbor : scene.Area(pt, 1))
            {
                if(scene.IsWater(neighbor))
                    continue;
                scene.SetOverlay(neighbor, Overlay::Sand);
                if(scene.Distance(neighbor) > scene.Distance(pond) && scene.Rnd().ByChance(50))
                    scene.Place(neighbor, shrubs);
            }
        }

        const Blob drift = scene.MakeBlob(scene.Step(center, dir + side, 4), 2.5);
        scene.Blend(drift, Ground::Lush);
        scene.ForBlob(drift, 0.85, 55, [&scene, &theme](const MapPoint& pt) {
            if(theme.flowers)
                scene.Place(pt, plants);
            else
                scene.Place(pt, theme.props);
        });
        scene.ForBlob(scene.MakeBlob(scene.Step(center, dir - side, 5), 2.1), 1., 65,
                      [&scene](const MapPoint& pt) { scene.PlaceVegetation(pt); });
        scene.ForSector(0., pi, 1, 3, 12, [&scene, &theme](const MapPoint& pt) { scene.Place(pt, theme.litter); });
    }

    /// Curved trail lined by remains which leads to the special object
    void CreateTrail(SceneBuilder& scene)
    {
        const Theme& theme = scene.GetTheme();
        const MapPoint center = scene.Center();
        const int dir = static_cast<int>(scene.Rnd().RandomValue(0, 5));
        const int turn = scene.Rnd().ByChance(50) ? 1 : -1;

        scene.Blend(scene.MakeBlob(center, 3.4), theme.surround);
        scene.Blend(scene.MakeBlob(center, theme.core == Ground::Rock ? 1.9 : 2.6), theme.core, true);
        if(theme.hill)
            scene.Bump(center, 2, 4.5);

        MapPoint pt = center;
        std::vector<std::pair<MapPoint, int>> trail;
        for(const int bend : {0, 0, 1, 1, 1, 2, 2})
        {
            pt = scene.Step(pt, dir + turn * bend);
            trail.emplace_back(pt, dir + turn * bend);
            scene.Block(pt);
        }
        for(const auto& node : trail)
            scene.Blend(scene.MakeBlob(node.first, 1.7, false), Ground::Dry);

        // Remains alternate on both sides of the trail
        for(unsigned i = 1; i < trail.size(); i++)
        {
            const int left = trail[i].second + 2;
            const int right = trail[i].second + 4;
            scene.Place(scene.Step(trail[i].first, i % 2 ? left : right), theme.props);
            if(scene.Rnd().ByChance(45))
                scene.Place(scene.Step(trail[i].first, i % 2 ? right : left), theme.litter);
        }
        scene.PlaceRock(scene.Step(trail.back().first, trail.back().second + 1));

        // Clumps of trees behind the special object and inside of the bend
        scene.ForBlob(scene.MakeBlob(scene.Step(center, dir + 3, 4), 2.4), 1., 65,
                      [&scene](const MapPoint& p) { scene.PlaceVegetation(p); });
        scene.ForBlob(scene.MakeBlob(scene.Step(trail[3].first, trail[3].second + turn * 2, 3), 1.9), 1., 60,
                      [&scene](const MapPoint& p) { scene.PlaceVegetation(p); });
        scene.ForSector(0., pi, 1, 3, 30, [&scene](const MapPoint& p) { scene.PlaceGroundCover(p); });
        scene.ForSector(0., pi, 2, 6, 8, [&scene](const MapPoint& p) { scene.Place(p, undergrowth); });
    }
} // namespace

bool IsSpecialObject(uint8_t objectType, uint8_t objectInfo)
{
    if(objectType != OT_Decoration)
        return false;
    return objectInfo == OI_Gate || objectInfo == OI_OpenGate || objectInfo == OI_Tent
           || objectInfo == OI_GuardhouseRuin || objectInfo == OI_TowerRuin || objectInfo == OI_CastleRuin
           || objectInfo == OI_Cross;
}

unsigned GetNumSpecialScenes(const MapExtent& size)
{
    return helpers::clamp((size.x + size.y) / 96u, 1u, 6u);
}

std::vector<MapPoint> FindSceneSites(const Map& map, RandomUtility& rnd, unsigned number)
{
    const auto isUnsuitable = [&map](const MapPoint& pt) {
        return !map.textureMap.All(pt, IsBuildableLand) || map.objectTypes[pt] != libsiedler2::OT_Empty
               || helpers::contains(map.barren, pt) || helpers::contains(map.reserved, pt);
    };
    const auto obstacleDistance = DistancesTo(map.size, isUnsuitable);

    std::vector<MapPoint> players = map.hqPositions;
    for(const Triangle& harbor : map.harbors)
        players.push_back(harbor.position);
    const auto playerDistance = DistancesTo(players, map.size);

    // Only the middle of a scene requires free land - one node more leaves a gap to the surroundings. Obstacles
    // further out cut the scene which allows scenes on maps with little free land.
    auto candidates = SelectPoints(
      [&](const MapPoint& pt) {
          return obstacleDistance[pt] > sceneCoreRadius + 1 && playerDistance[pt] >= minPlayerDistance;
      },
      map.size);

    const unsigned minSceneDistance = std::max<unsigned>(4 * sceneRadius, (map.size.x + map.size.y) / 16u);
    std::vector<MapPoint> sites;

    while(sites.size() < number && !candidates.empty())
    {
        const MapPoint site = rnd.RandomItem(candidates);
        sites.push_back(site);
        helpers::erase_if(candidates, [&map, &site, minSceneDistance](const MapPoint& pt) {
            return map.z.CalcDistance(site, pt) < minSceneDistance;
        });
    }

    return sites;
}

void CreateScene(Map& map, RandomUtility& rnd, const MapPoint& center, SceneType type, SceneStyle style)
{
    SceneBuilder scene(map, rnd, center, type);
    scene.PlaceCore(type);

    switch(style)
    {
        case SceneStyle::Overgrown: CreateOvergrown(scene); break;
        case SceneStyle::Avenue: CreateAvenue(scene); break;
        case SceneStyle::Crag: CreateCrag(scene); break;
        case SceneStyle::Trail: CreateTrail(scene); break;
    }

    scene.Finish();
}

std::vector<Scene> AddSpecialScenes(Map& map, RandomUtility& rnd)
{
    // Each type is used once before any type is repeated
    std::vector<SceneType> types;
    for(const SceneType type : helpers::EnumRange<SceneType>())
        types.push_back(type);
    for(size_t i = types.size(); i > 1; i--)
        std::swap(types[i - 1], types[rnd.Index(i)]);

    std::vector<Scene> scenes;
    for(const MapPoint& site : FindSceneSites(map, rnd, GetNumSpecialScenes(map.size)))
    {
        const SceneType type = types[scenes.size() % types.size()];
        const auto style =
          static_cast<SceneStyle>(rnd.RandomValue(0u, static_cast<unsigned>(maxEnumValue(SceneStyle{}))));
        CreateScene(map, rnd, site, type, style);
        scenes.push_back(Scene{type, style, site});
    }

    return scenes;
}

} // namespace rttr::mapGenerator
