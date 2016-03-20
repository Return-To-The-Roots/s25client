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

///////////////////////////////////////////////////////////////////////////////
// Header


#include "defines.h" // IWYU pragma: keep
#include "noBaseBuilding.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "ai/AIEvents.h"
#include "nodeObjs/noExtension.h"
#include "nodeObjs/noFlag.h"
#include "gameData/DoorConsts.h"
#include "gameData/MapConsts.h"
#include "nobBaseWarehouse.h"
#include "Ware.h"
#include "SerializedGameData.h"
#include "Loader.h"
#include "GameInterface.h"
#include "Log.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

noBaseBuilding::noBaseBuilding(const NodalObjectType nop, const BuildingType type, const MapPoint pos, const unsigned char player)
    : noRoadNode(nop, pos, player), type_(type), nation(GAMECLIENT.GetPlayer(player).nation), door_point_x(1000000), door_point_y(DOOR_CONSTS[GAMECLIENT.GetPlayer(player).nation][type])
{
    MapPoint flagPt = gwg->GetNeighbour(pos, 4);
    // Evtl Flagge setzen, wenn noch keine da ist
    if(gwg->GetNO(flagPt)->GetType() != NOP_FLAG)
    {
        gwg->DestroyNO(flagPt, false);
        gwg->SetNO(flagPt, new noFlag(flagPt, player));
    }

    // Straßeneingang setzen (wenn nicht schon vorhanden z.b. durch vorherige Baustelle!)
    if(!gwg->GetPointRoad(pos, 4))
    {
        gwg->SetPointRoad(pos, 4, 1);

        // Straßenverbindung erstellen zwischen Flagge und Haus
        // immer von Flagge ZU Gebäude (!)
        std::vector<unsigned char> route(1, 1);
        // Straße zuweisen
        gwg->GetSpecObj<noRoadNode>(flagPt)->routes[1] = // der Flagge
            routes[4] = // dem Gebäude
                new RoadSegment(RoadSegment::RT_NORMAL, gwg->GetSpecObj<noRoadNode>(flagPt), this, route);
    }
    else
    {
        // vorhandene Straße der Flagge nutzen
        noFlag* flag = gwg->GetSpecObj<noFlag>(flagPt);

        RTTR_Assert(flag->routes[1]);
        routes[4] = flag->routes[1];
        routes[4]->SetF2(this);
    }

    // Werde/Bin ich (mal) ein großes Schloss? Dann müssen die Anbauten gesetzt werden
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            MapPoint pos2 = gwg->GetNeighbour(pos, i);
            gwg->DestroyNO(pos2, false);
            gwg->SetNO(pos2, new noExtension(this));
        }
    }
}

noBaseBuilding::~noBaseBuilding()
{
}

void noBaseBuilding::Destroy_noBaseBuilding()
{
    DestroyAllRoads();
    //notify the ai about this
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingDestroyed, pos, type_), player);

    if(gwg->GetGameInterface())
        gwg->GetGameInterface()->GI_UpdateMinimap(pos);

    // evtl Anbauten wieder abreißen
    DestroyBuildingExtensions();

    // ggf. Fenster schließen
    gwg->ImportantObjectDestroyed(pos);

    // Baukosten zurückerstatten (nicht bei Baustellen)
    const GlobalGameSettings& settings = GAMECLIENT.GetGGS();
    if( (GetGOT() != GOT_BUILDINGSITE) &&
            ( settings.isEnabled(AddonId::REFUND_MATERIALS) ||
              settings.isEnabled(AddonId::REFUND_ON_EMERGENCY) ) )
    {
        // lebt unsere Flagge noch?
        noFlag* flag = GetFlag();
        if(flag)
        {
            unsigned int percent_index = 0;

            // wenn Rückerstattung aktiv ist, entsprechende Prozentzahl wählen
            if(settings.isEnabled(AddonId::REFUND_MATERIALS))
                percent_index = settings.getSelection(AddonId::REFUND_MATERIALS);
            // wenn Rückerstattung bei Notprogramm aktiv ist, 50% zurückerstatten
            else if(gwg->GetPlayer(player).hasEmergency() && settings.isEnabled(AddonId::REFUND_ON_EMERGENCY))
                percent_index = 2;

            // wieviel kriegt man von jeder Ware wieder?
            const unsigned int percents[5] = { 0, 25, 50, 75, 100 };
            const unsigned int percent = 10 * percents[percent_index];

            // zurückgaben berechnen (abgerundet)
            unsigned int boards = (percent * BUILDING_COSTS[nation][type_].boards) / 1000;
            unsigned int stones = (percent * BUILDING_COSTS[nation][type_].stones) / 1000;

            GoodType goods[2] = {GD_BOARDS, GD_STONES};
            bool which = 0;
            while(flag->IsSpaceForWare() && ( boards > 0 || stones > 0 ))
            {
                if( (!which && boards > 0) || (which && stones > 0))
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

    sgd.PushUnsignedChar(static_cast<unsigned char>(type_));
    sgd.PushUnsignedChar(nation);
    sgd.PushSignedInt(door_point_x);
    sgd.PushSignedInt(door_point_y);
}

noBaseBuilding::noBaseBuilding(SerializedGameData& sgd, const unsigned obj_id) : noRoadNode(sgd, obj_id),
    type_(BuildingType(sgd.PopUnsignedChar())),
    nation(Nation(sgd.PopUnsignedChar())),
    door_point_x(sgd.PopSignedInt()),
    door_point_y(sgd.PopSignedInt())
{
}

int noBaseBuilding::GetDoorPointX()
{
    // Did we calculate the door point yet?
    if(door_point_x == 1000000)
    {
        // The door is on the line between the building and flag point. The position of the line is set by the y-offset
        // this is why we need the x-offset here according to the equation x = m*y + n
        // with n=0 (as door point is relative to building pos) and m = dx/dy
        const Point<int> bldPos  = Point<int>(gwg->GetNodePos(pos));
        const Point<int> flagPos = Point<int>(gwg->GetNodePos(gwg->GetNeighbour(pos, 4)));
        Point<int> diff = flagPos - bldPos;

        // We could have crossed the map border which results in unreasonable diffs
        // clamp the diff to [-w/2,w/2],[-h/2, h/2] (maximum diffs)
        const int mapWidth  = gwg->GetWidth()  * TR_W;
        const int mapHeight = gwg->GetHeight() * TR_H;

        if(diff.x < -mapWidth/2)
            diff.x += mapWidth;
        else if(diff.x > mapWidth/2)
            diff.x -= mapWidth;
        if(diff.y < -mapHeight/2)
            diff.y += mapHeight;
        else if(diff.y > mapHeight/2)
            diff.y -= mapHeight;

        door_point_x = (door_point_y * diff.x) / diff.y;
    }

    return door_point_x;
}

noFlag* noBaseBuilding::GetFlag() const
{
    return gwg->GetSpecObj<noFlag>(gwg->GetNeighbour(pos, 4));
}


void noBaseBuilding::WareNotNeeded(Ware* ware)
{
    if (!ware)
    {
        RTTR_Assert(false);
        LOG.lprintf("Warning: Trying to remove non-existing ware. Please report this replay!\n");
        return;
    }

    if(ware->IsWaitingInWarehouse())
    {
        // Bestellung im Lagerhaus stornieren
        static_cast<nobBaseWarehouse*>(ware->GetLocation())->CancelWare(ware);
        // Ware muss auch noch vernichtet werden!
        // Inventur entsprechend verringern
        //GAMECLIENT.GetPlayer(player).DecreaseInventoryWare(ware->type,1);
        GAMECLIENT.GetPlayer(player).RemoveWare(ware);
        delete ware;
    }
    else
        ware->GoalDestroyed();
}


void noBaseBuilding::DestroyBuildingExtensions()
{
    // Nur bei großen Gebäuden gibts diese Anbauten
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            gwg->DestroyNO(gwg->GetNeighbour(pos, i));
        }

    }
}

noBase::BlockingManner noBaseBuilding::GetBM() const
{
    BuildingQuality bq = GetSize();

    if(bq == BQ_HARBOR)
        return noBase::BM_CASTLE;
    else
        return noBase::BlockingManner(unsigned(bq) - 1);
}

/// Gibt ein Bild zurück für das normale Gebäude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImage() const
{
    if (type_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 6 : 1));
    else
        return LOADER.GetNationImage(nation, 250 + 5 * type_);
}

/// Gibt ein Bild zurück für das Gebäudegerüst
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImage() const
{
    if (type_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 3);
    else
        return LOADER.GetNationImage(nation, 250 + 5 * type_ + 2);
}

/// Gibt ein Bild zurück für das normale Gebäude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImageShadow() const
{
    if (type_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 2);
    else
        return LOADER.GetNationImage(nation, 250 + 5 * type_ + 1);
}

/// Gibt ein Bild zurück für das Gebäudegerüst
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImageShadow() const
{
    if (type_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 4);
    else
        return LOADER.GetNationImage(nation, 250 + 5 * type_ + 3);
}

/// Gibt ein Bild zurück für die Tür des Gebäudes
glArchivItem_Bitmap* noBaseBuilding::GetDoorImage() const
{
    if (type_ == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 7 : 5));
    else
        return LOADER.GetNationImage(nation, 250 + 5 * type_ + 4);
}

