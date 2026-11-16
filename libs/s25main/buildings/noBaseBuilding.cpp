// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noBaseBuilding.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "addons/const_addons.h"
#include "nobBaseWarehouse.h"
#include "notifications/BuildingNote.h"
#include "world/GameWorld.h"
#include "nodeObjs/noExtension.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingConsts.h"
#include "gameData/DoorConsts.h"
#include "gameData/MapConsts.h"
#include "s25util/Log.h"

noBaseBuilding::noBaseBuilding(const NodalObjectType nop, const BuildingType type, const MapPoint pos,
                               const unsigned char player)
    : noRoadNode(nop, pos, player), bldType_(type), nation(world->getPlayer(player).nation), door_point_x(1000000),
      door_point_y(DOOR_CONSTS[world->getPlayer(player).nation][type])
{
    MapPoint flagPt = getFlagPos();
    // Evtl Flagge setzen, wenn noch keine da ist
    if(world->getNO(flagPt)->getType() != NodalObjectType::Flag)
    {
        world->destroyNO(flagPt, false);
        world->setNO(flagPt, new noFlag(flagPt, player));
    }

    // Straßeneingang setzen (wenn nicht schon vorhanden z.b. durch vorherige Baustelle!)
    if(world->getPointRoad(pos, Direction::SouthEast) == PointRoad::None)
    {
        world->setPointRoad(pos, Direction::SouthEast, PointRoad::Normal);

        // Straßenverbindung erstellen zwischen Flagge und Haus
        // immer von Flagge ZU Gebäude (!)
        std::vector<Direction> route(1, Direction::NorthWest);
        // Straße zuweisen
        auto* rs = new RoadSegment(RoadType::Normal, world->getSpecObj<noRoadNode>(flagPt), this, route);
        world->getSpecObj<noRoadNode>(flagPt)->setRoute(Direction::NorthWest, rs); // der Flagge
        setRoute(Direction::SouthEast, rs);                                        // dem Gebäude
    } else
    {
        // vorhandene Straße der Flagge nutzen
        auto* flag = world->getSpecObj<noFlag>(flagPt);

        RTTR_Assert(flag->getRoute(Direction::NorthWest));
        setRoute(Direction::SouthEast, flag->getRoute(Direction::NorthWest));
        getRoute(Direction::SouthEast)->setF2(this);
    }

    // Werde/Bin ich (mal) ein großes Schloss? Dann müssen die Anbauten gesetzt werden
    if(getSize() == BuildingQuality::Castle || getSize() == BuildingQuality::Harbor)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            const MapPoint neighbor = world->getNeighbour(pos, i);

            if(type == BuildingType::Headquarters)
            {
                const NodalObjectType neighborNoType = world->getNO(neighbor)->getType();
                // Don't replace nearby static objects or trees. Needed for "build headquarters" cheat to work like in
                // the original. This situation shouldn't happen any other way (can't normally build big buildings right
                // next to static objects or trees). Trees which be remain because of this will be replaced by
                // extensions instead of stumps if they are cut while still right next to the HQ.
                if(neighborNoType == NodalObjectType::Object || neighborNoType == NodalObjectType::Tree)
                    continue;
            }

            world->destroyNO(neighbor, false);
            world->setNO(neighbor, new noExtension(this));
        }
    }
}

noBaseBuilding::~noBaseBuilding() = default;

void noBaseBuilding::destroy()
{
    destroyAllRoads();
    world->getNotifications().publish(BuildingNote(BuildingNote::Destroyed, player, pos, bldType_));

    if(world->getGameInterface())
        world->getGameInterface()->GI_UpdateMinimap(pos);

    // evtl Anbauten wieder abreißen
    destroyBuildingExtensions();

    // Baukosten zurückerstatten (nicht bei Baustellen)
    const GlobalGameSettings& settings = world->getGGS();
    if((getGOT() != GO_Type::Buildingsite)
       && (settings.isEnabled(AddonId::REFUND_MATERIALS) || settings.isEnabled(AddonId::REFUND_ON_EMERGENCY)))
    {
        // lebt unsere Flagge noch?
        noFlag* flag = getFlag();
        if(flag)
        {
            unsigned percent_index = 0;

            // wenn Rückerstattung aktiv ist, entsprechende Prozentzahl wählen
            if(settings.isEnabled(AddonId::REFUND_MATERIALS))
                percent_index = settings.getSelection(AddonId::REFUND_MATERIALS);
            // wenn Rückerstattung bei Notprogramm aktiv ist, 50% zurückerstatten
            else if(world->getPlayer(player).hasEmergency() && settings.isEnabled(AddonId::REFUND_ON_EMERGENCY))
                percent_index = 2;

            // wieviel kriegt man von jeder Ware wieder?
            const std::array<unsigned, 5> percents = {0, 25, 50, 75, 100};
            const unsigned percent = 10 * percents[percent_index];

            // zurückgaben berechnen (abgerundet)
            unsigned boards = (percent * BUILDING_COSTS[bldType_].boards) / 1000;
            unsigned stones = (percent * BUILDING_COSTS[bldType_].stones) / 1000;

            std::array<GoodType, 2> goods = {GoodType::Boards, GoodType::Stones};
            bool which = false;
            while(flag->hasSpaceForWare() && (boards > 0 || stones > 0))
            {
                if((!which && boards > 0) || (which && stones > 0))
                {
                    // Ware erzeugen
                    auto ware = std::make_unique<Ware>(goods[which], nullptr, flag);
                    ware->waitAtFlag(*flag);
                    // Inventur anpassen
                    world->getPlayer(player).increaseInventoryWare(goods[which], 1);
                    // Abnehmer für Ware finden
                    ware->setGoal(world->getPlayer(player).findClientForWare(*ware));
                    // Ware soll ihren weiteren Weg berechnen
                    ware->recalcRoute();
                    // Ware ablegen
                    flag->addWare(std::move(ware));

                    if(!which)
                        --boards;
                    else
                        --stones;
                }

                which = !which;
            }
        }
    }

    noRoadNode::destroy();
}

void noBaseBuilding::Serialize(SerializedGameData& sgd) const
{
    noRoadNode::Serialize(sgd);

    sgd.pushEnum<uint8_t>(bldType_);
    sgd.pushEnum<uint8_t>(nation);
    sgd.PushSignedInt(door_point_x);
    sgd.PushSignedInt(door_point_y);
}

noBaseBuilding::noBaseBuilding(SerializedGameData& sgd, const unsigned obj_id)
    : noRoadNode(sgd, obj_id), bldType_(sgd.pop<BuildingType>()), nation(sgd.pop<Nation>()),
      door_point_x(sgd.PopSignedInt()), door_point_y(sgd.PopSignedInt())
{}

int noBaseBuilding::getDoorPointX()
{
    // Did we calculate the door point yet?
    if(door_point_x == 1000000)
    {
        // The door is on the line between the building and flag point. The position of the line is set by the y-offset
        // this is why we need the x-offset here according to the equation x = m*y + n
        // with n=0 (as door point is relative to building pos) and m = dx/dy
        const Position bldPos = world->getNodePos(pos);
        const Position flagPos = world->getNodePos(getFlagPos());
        Position diff = flagPos - bldPos;

        // We could have crossed the map border which results in unreasonable diffs
        // clamp the diff to [-w/2,w/2],[-h/2, h/2] (maximum diffs)
        const int mapWidth = world->getWidth() * TR_W;
        const int mapHeight = world->getHeight() * TR_H;

        if(diff.x < -mapWidth / 2)
            diff.x += mapWidth;
        else if(diff.x > mapWidth / 2)
            diff.x -= mapWidth;
        if(diff.y < -mapHeight / 2)
            diff.y += mapHeight;
        else if(diff.y > mapHeight / 2)
            diff.y -= mapHeight;

        door_point_x = (door_point_y * diff.x) / diff.y;
    }

    return door_point_x;
}

noFlag* noBaseBuilding::getFlag() const
{
    return world->getSpecObj<noFlag>(getFlagPos());
}

MapPoint noBaseBuilding::getFlagPos() const
{
    return world->getNeighbour(pos, Direction::SouthEast);
}

bool noBaseBuilding::isConnected() const
{
    // Harbors are always considered connected via sea
    if(getBuildingType() == BuildingType::HarborBuilding)
        return true;

    const helpers::EnumArray<RoadSegment*, Direction>& routes = getFlag()->getRoutes();
    // Check paths in all directions except back to building
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(dir != Direction::NorthWest && routes[dir])
            return true;
    }

    return false;
}

void noBaseBuilding::wareNotNeeded(Ware* ware)
{
    if(!ware)
    {
        RTTR_Assert(false);
        LOG.write("Warning: Trying to remove non-existing ware. Please report this replay!\n");
        return;
    }

    if(ware->isWaitingInWarehouse())
    {
        // Bestellung im Lagerhaus stornieren
        world->getPlayer(player).removeWare(*ware);
        static_cast<nobBaseWarehouse*>(ware->getLocation())->cancelWare(ware);
    } else
        ware->goalDestroyed();
}

void noBaseBuilding::destroyBuildingExtensions()
{
    // Nur bei großen Gebäuden gibts diese Anbauten
    if(getSize() == BuildingQuality::Castle || getSize() == BuildingQuality::Harbor)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            const MapPoint neighbor = world->getNeighbour(pos, i);
            if(world->getNO(neighbor)->getType() == NodalObjectType::Extension)
                world->destroyNO(neighbor);
        }
    }
}

BuildingQuality noBaseBuilding::getSize() const
{
    return BUILDING_SIZE[bldType_];
}

BlockingManner noBaseBuilding::getBM() const
{
    return BlockingManner::Building;
}

/// Gibt ein Bild zurück für das normale Gebäude
ITexture& noBaseBuilding::getBuildingImage() const
{
    return getBuildingImage(bldType_, nation);
}

ITexture& noBaseBuilding::getBuildingImage(BuildingType type, Nation nation) //-V688
{
    return LOADER.building_cache[nation][type].building;
}

/// Gibt ein Bild zurück für die Tür des Gebäudes
ITexture& noBaseBuilding::getDoorImage() const
{
    return LOADER.building_cache[nation][bldType_].door;
}
