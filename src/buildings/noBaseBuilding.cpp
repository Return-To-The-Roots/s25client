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

#include "rttrDefines.h" // IWYU pragma: keep
#include "noBaseBuilding.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "addons/const_addons.h"
#include "network/GameClient.h"
#include "nobBaseWarehouse.h"
#include "notifications/BuildingNote.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noExtension.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingConsts.h"
#include "gameData/DoorConsts.h"
#include "gameData/MapConsts.h"
#include "libutil/Log.h"

noBaseBuilding::noBaseBuilding(const NodalObjectType nop, const BuildingType type, const MapPoint pos, const unsigned char player)
    : noRoadNode(nop, pos, player), bldType_(type), nation(gwg->GetPlayer(player).nation), door_point_x(1000000),
      door_point_y(DOOR_CONSTS[gwg->GetPlayer(player).nation][type])
{
    MapPoint flagPt = GetFlagPos();
    // Evtl Flagge setzen, wenn noch keine da ist
    if(gwg->GetNO(flagPt)->GetType() != NOP_FLAG)
    {
        gwg->DestroyNO(flagPt, false);
        gwg->SetNO(flagPt, new noFlag(flagPt, player));
    }

    // Straßeneingang setzen (wenn nicht schon vorhanden z.b. durch vorherige Baustelle!)
    if(!gwg->GetPointRoad(pos, Direction::SOUTHEAST))
    {
        gwg->SetPointRoad(pos, Direction::SOUTHEAST, RoadSegment::RT_NORMAL + 1);

        // Straßenverbindung erstellen zwischen Flagge und Haus
        // immer von Flagge ZU Gebäude (!)
        std::vector<Direction> route(1, Direction::NORTHWEST);
        // Straße zuweisen
        RoadSegment* rs = new RoadSegment(RoadSegment::RT_NORMAL, gwg->GetSpecObj<noRoadNode>(flagPt), this, route);
        gwg->GetSpecObj<noRoadNode>(flagPt)->SetRoute(Direction::NORTHWEST, rs); // der Flagge
        SetRoute(Direction::SOUTHEAST, rs);                                      // dem Gebäude
    } else
    {
        // vorhandene Straße der Flagge nutzen
        noFlag* flag = gwg->GetSpecObj<noFlag>(flagPt);

        RTTR_Assert(flag->GetRoute(Direction::NORTHWEST));
        SetRoute(Direction::SOUTHEAST, flag->GetRoute(Direction::NORTHWEST));
        GetRoute(Direction::SOUTHEAST)->SetF2(this);
    }

    // Werde/Bin ich (mal) ein großes Schloss? Dann müssen die Anbauten gesetzt werden
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            MapPoint pos2 = gwg->GetNeighbour(pos, Direction::fromInt(i));
            gwg->DestroyNO(pos2, false);
            gwg->SetNO(pos2, new noExtension(this));
        }
    }
}

noBaseBuilding::~noBaseBuilding() {}

void noBaseBuilding::Destroy_noBaseBuilding()
{
    DestroyAllRoads();
    gwg->GetNotifications().publish(BuildingNote(BuildingNote::Destroyed, player, pos, bldType_));

    if(gwg->GetGameInterface())
        gwg->GetGameInterface()->GI_UpdateMinimap(pos);

    // evtl Anbauten wieder abreißen
    DestroyBuildingExtensions();

    // Baukosten zurückerstatten (nicht bei Baustellen)
    const GlobalGameSettings& settings = gwg->GetGGS();
    if((GetGOT() != GOT_BUILDINGSITE)
       && (settings.isEnabled(AddonId::REFUND_MATERIALS) || settings.isEnabled(AddonId::REFUND_ON_EMERGENCY)))
    {
        // lebt unsere Flagge noch?
        noFlag* flag = GetFlag();
        if(flag)
        {
            unsigned percent_index = 0;

            // wenn Rückerstattung aktiv ist, entsprechende Prozentzahl wählen
            if(settings.isEnabled(AddonId::REFUND_MATERIALS))
                percent_index = settings.getSelection(AddonId::REFUND_MATERIALS);
            // wenn Rückerstattung bei Notprogramm aktiv ist, 50% zurückerstatten
            else if(gwg->GetPlayer(player).hasEmergency() && settings.isEnabled(AddonId::REFUND_ON_EMERGENCY))
                percent_index = 2;

            // wieviel kriegt man von jeder Ware wieder?
            const unsigned percents[5] = {0, 25, 50, 75, 100};
            const unsigned percent = 10 * percents[percent_index];

            // zurückgaben berechnen (abgerundet)
            unsigned boards = (percent * BUILDING_COSTS[nation][bldType_].boards) / 1000;
            unsigned stones = (percent * BUILDING_COSTS[nation][bldType_].stones) / 1000;

            GoodType goods[2] = {GD_BOARDS, GD_STONES};
            bool which = 0;
            while(flag->IsSpaceForWare() && (boards > 0 || stones > 0))
            {
                if((!which && boards > 0) || (which && stones > 0))
                {
                    // Ware erzeugen
                    Ware* ware = new Ware(goods[which], NULL, flag);
                    // Inventur anpassen
                    gwg->GetPlayer(player).IncreaseInventoryWare(goods[which], 1);
                    // Abnehmer für Ware finden
                    ware->SetGoal(gwg->GetPlayer(player).FindClientForWare(ware));
                    // Ware soll ihren weiteren Weg berechnen
                    ware->RecalcRoute();
                    // Ware ablegen
                    flag->AddWare(ware);
                    ware->WaitAtFlag(flag);

                    if(!which)
                        --boards;
                    else
                        --stones;
                }

                which = !which;
            }
        }
    }

    Destroy_noRoadNode();
}

void noBaseBuilding::Serialize_noBaseBuilding(SerializedGameData& sgd) const
{
    Serialize_noRoadNode(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(bldType_));
    sgd.PushUnsignedChar(nation);
    sgd.PushSignedInt(door_point_x);
    sgd.PushSignedInt(door_point_y);
}

noBaseBuilding::noBaseBuilding(SerializedGameData& sgd, const unsigned obj_id)
    : noRoadNode(sgd, obj_id), bldType_(BuildingType(sgd.PopUnsignedChar())), nation(Nation(sgd.PopUnsignedChar())),
      door_point_x(sgd.PopSignedInt()), door_point_y(sgd.PopSignedInt())
{}

int noBaseBuilding::GetDoorPointX()
{
    // Did we calculate the door point yet?
    if(door_point_x == 1000000)
    {
        // The door is on the line between the building and flag point. The position of the line is set by the y-offset
        // this is why we need the x-offset here according to the equation x = m*y + n
        // with n=0 (as door point is relative to building pos) and m = dx/dy
        const Point<int> bldPos = gwg->GetNodePos(pos);
        const Point<int> flagPos = gwg->GetNodePos(GetFlagPos());
        Point<int> diff = flagPos - bldPos;

        // We could have crossed the map border which results in unreasonable diffs
        // clamp the diff to [-w/2,w/2],[-h/2, h/2] (maximum diffs)
        const int mapWidth = gwg->GetWidth() * TR_W;
        const int mapHeight = gwg->GetHeight() * TR_H;

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

noFlag* noBaseBuilding::GetFlag() const
{
    return gwg->GetSpecObj<noFlag>(GetFlagPos());
}

MapPoint noBaseBuilding::GetFlagPos() const
{
    return gwg->GetNeighbour(pos, Direction::SOUTHEAST);
}

void noBaseBuilding::WareNotNeeded(Ware* ware)
{
    if(!ware)
    {
        RTTR_Assert(false);
        LOG.write("Warning: Trying to remove non-existing ware. Please report this replay!\n");
        return;
    }

    if(ware->IsWaitingInWarehouse())
    {
        // Bestellung im Lagerhaus stornieren
        static_cast<nobBaseWarehouse*>(ware->GetLocation())->CancelWare(ware);
        // Ware muss auch noch vernichtet werden!
        // Inventur entsprechend verringern
        gwg->GetPlayer(player).RemoveWare(ware);
        delete ware;
    } else
        ware->GoalDestroyed();
}

void noBaseBuilding::DestroyBuildingExtensions()
{
    // Nur bei großen Gebäuden gibts diese Anbauten
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            gwg->DestroyNO(gwg->GetNeighbour(pos, Direction::fromInt(i)));
        }
    }
}

BuildingQuality noBaseBuilding::GetSize() const
{
    return BUILDING_SIZE[bldType_];
}

BlockingManner noBaseBuilding::GetBM() const
{
    return BlockingManner::Building;
}

/// Gibt ein Bild zurück für das normale Gebäude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImage() const
{
    return GetBuildingImage(bldType_, nation, gwg->GetLandscapeType());
}

glArchivItem_Bitmap* noBaseBuilding::GetBuildingImage(BuildingType type, Nation nation) //-V688
{
    return GetBuildingImage(type, nation, LandscapeType(LOADER.GetLastGFX()));
}

glArchivItem_Bitmap* noBaseBuilding::GetBuildingImage(BuildingType type, Nation nation, LandscapeType lt) //-V688
{
    if(type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((lt == LT_WINTERWORLD) ? 6 : 1));
    else
        return LOADER.GetNationImage(nation, 250 + 5 * type);
}

/// Gibt ein Bild zurück für das Gebäudegerüst
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImage() const
{
    if(bldType_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 3);
    else
        return LOADER.GetNationImage(nation, 250 + 5 * bldType_ + 2);
}

/// Gibt ein Bild zurück für das normale Gebäude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImageShadow() const
{
    if(bldType_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 2);
    else
        return LOADER.GetNationImage(nation, 250 + 5 * bldType_ + 1);
}

/// Gibt ein Bild zurück für das Gebäudegerüst
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImageShadow() const
{
    if(bldType_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 4);
    else
        return LOADER.GetNationImage(nation, 250 + 5 * bldType_ + 3);
}

/// Gibt ein Bild zurück für die Tür des Gebäudes
glArchivItem_Bitmap* noBaseBuilding::GetDoorImage() const
{
    if(bldType_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 7 : 5));
    else
        return LOADER.GetNationImage(nation, 250 + 5 * bldType_ + 4);
}
