// $Id: noBaseBuilding.cpp 9402 2014-05-10 06:54:13Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your oposion) any later version.
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


#include "defines.h"
#include "noBaseBuilding.h"
#include "GameWorld.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "gameData/DoorConsts.h"
#include "gameData/MapConsts.h"
#include "nodeObjs/noExtension.h"
#include "Random.h"
#include "nobBaseWarehouse.h"
#include "Ware.h"
#include "WindowManager.h"
#include "SerializedGameData.h"
#include "GameInterface.h"
#include <iostream>


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

noBaseBuilding::noBaseBuilding(const NodalObjectType nop, const BuildingType type, const MapPoint pos, const unsigned char player)
    : noRoadNode(nop, pos, player), type(type), nation(GAMECLIENT.GetPlayer(player)->nation), door_point_x(1000000), door_point_y(DOOR_CONSTS[GAMECLIENT.GetPlayer(player)->nation][type])
{

    // Evtl Flagge setzen, wenn noch keine da ist
    if(gwg->GetNO(gwg->GetNeighbour(pos, 4))->GetType() != NOP_FLAG)
    {
        // ggf. vorherige Objekte löschen
        noBase* no = gwg->GetSpecObj<noBase>(gwg->GetNeighbour(pos, 4));
        if(no)
        {
            no->Destroy();
            delete no;
        }
        gwg->SetNO(new noFlag(gwg->GetNeighbour(pos, 4), player), gwg->GetNeighbour(pos, 4));
    }

    // Straßeneingang setzen (wenn nicht schon vorhanden z.b. durch vorherige Baustelle!)
    if(!gwg->GetPointRoad(pos, 4))
    {
        gwg->SetPointRoad(pos, 4, 1);

        // Straßenverbindung erstellen zwischen Flagge und Haus
        // immer von Flagge ZU Gebäude (!)
        std::vector<unsigned char> route(1, 1);
        // Straße zuweisen
        gwg->GetSpecObj<noRoadNode>(gwg->GetNeighbour(pos, 4))->routes[1] = // der Flagge
            routes[4] = // dem Gebäude
                new RoadSegment(RoadSegment::RT_NORMAL, gwg->GetSpecObj<noRoadNode>(gwg->GetNeighbour(pos, 4)), this, route);
    }
    else
    {
        // vorhandene Straße der Flagge nutzen
        noFlag* flag = gwg->GetSpecObj<noFlag>(gwg->GetNeighbour(pos, 4));

        assert(flag->routes[1]);
        routes[4] = flag->routes[1];
        routes[4]->SetF2(this);

    }

    // Werde/Bin ich (mal) ein großes Schloss? Dann müssen die Anbauten gesetzt werden
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            MapPoint pos2 = gwg->GetNeighbour(pos, i);

            noBase* no = gwg->GetSpecObj<noBase>(pos2);
            if(no)
            {
                no->Destroy();
                delete no;
            }
            gwg->SetNO(new noExtension(this), pos2);
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
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingDestroyed, pos, type), player);

    if(gwg->GetGameInterface())
        gwg->GetGameInterface()->GI_UpdateMinimap(pos);

    // evtl Anbauten wieder abreißen
    DestroyBuildingExtensions();

    // ggf. Fenster schließen
    gwg->ImportantObjectDestroyed(pos);

    // Baukosten zurückerstatten (nicht bei Baustellen)
    if( (GetGOT() != GOT_BUILDINGSITE) &&
            ( GAMECLIENT.GetGGS().isEnabled(ADDON_REFUND_MATERIALS) ||
              GAMECLIENT.GetGGS().isEnabled(ADDON_REFUND_ON_EMERGENCY) ) )
    {
        // lebt unsere Flagge noch?
        noFlag* flag = GetFlag();
        if(flag)
        {
            unsigned int percent_index = 0;

            // wenn Rückerstattung aktiv ist, entsprechende Prozentzahl wählen
            if(GAMECLIENT.GetGGS().isEnabled(ADDON_REFUND_MATERIALS))
                percent_index = GAMECLIENT.GetGGS().getSelection(ADDON_REFUND_MATERIALS);

            // wenn Rückerstattung bei Notprogramm aktiv ist, 50% zurückerstatten
            else if(gwg->GetPlayer(player)->hasEmergency() && GAMECLIENT.GetGGS().isEnabled(ADDON_REFUND_ON_EMERGENCY))
                percent_index = 2;

            // wieviel kriegt man von jeder Ware wieder?
            const unsigned int percents[5] = { 0, 25, 50, 75, 100 };
            const unsigned int percent = 10 * percents[percent_index];

            // zurückgaben berechnen (abgerundet)
            unsigned int boards = (percent * BUILDING_COSTS[nation][type].boards) / 1000;
            unsigned int stones = (percent * BUILDING_COSTS[nation][type].stones) / 1000;

            GoodType goods[2] = {GD_BOARDS, GD_STONES};
            bool which = 0;
            while(flag->IsSpaceForWare() && ( boards > 0 || stones > 0 ))
            {
                if( (!which && boards > 0) || (which && stones > 0))
                {
                    // Ware erzeugen
                    Ware* ware = new Ware(goods[which], 0, flag);
                    // Inventur anpassen
                    gwg->GetPlayer(player)->IncreaseInventoryWare(goods[which], 1);
                    // Abnehmer für Ware finden
                    ware->goal = gwg->GetPlayer(player)->FindClientForWare(ware);
                    // Ware soll ihren weiteren Weg berechnen
                    ware->RecalcRoute();
                    // Ware ablegen
                    flag->AddWare(ware);
                    ware->LieAtFlag(flag);

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

void noBaseBuilding::Serialize_noBaseBuilding(SerializedGameData* sgd) const
{
    Serialize_noRoadNode(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(type));
    sgd->PushUnsignedChar(nation);
    sgd->PushSignedInt(door_point_x);
    sgd->PushSignedInt(door_point_y);
}

noBaseBuilding::noBaseBuilding(SerializedGameData* sgd, const unsigned obj_id) : noRoadNode(sgd, obj_id),
    type(BuildingType(sgd->PopUnsignedChar())),
    nation(Nation(sgd->PopUnsignedChar())),
    door_point_x(sgd->PopSignedInt()),
    door_point_y(sgd->PopSignedInt())
{
}

short noBaseBuilding::GetDoorPointX()
{
    // Door-Points ausrechnen (Punkte, bis wohin die Menschen gehen, wenn sie ins
    // Gebäude gehen, woa ußerdem der Bauarbeiter baut und wo die Waren liegen

    // Door-Point noch nicht ausgerechnet?
    if(door_point_x == 1000000)
    {
        int x1 = static_cast<int>(gwg->GetTerrainX(pos));
        int y1 = static_cast<int>(gwg->GetTerrainY(pos));
        int x2 = static_cast<int>(gwg->GetTerrainX(gwg->GetNeighbour(pos, 4)));
        int y2 = static_cast<int>(gwg->GetTerrainY(gwg->GetNeighbour(pos, 4)));

        // Gehen wir über einen Kartenrand (horizontale Richung?)
        if(std::abs(x1 - x2) >= gwg->GetWidth() * TR_W / 2)
        {
            if(std::abs(x1 - int(gwg->GetWidth())*TR_W - x2) < std::abs(x1 - x2))
                x1 -= gwg->GetWidth() * TR_W;
            else
                x1 += gwg->GetWidth() * TR_W;
        }
        // Und dasselbe für vertikale Richtung
        if(std::abs(y1 - y2) >= gwg->GetHeight() * TR_H / 2)
        {
            if(std::abs(y1 - int(gwg->GetHeight())*TR_H - y2) < std::abs(y1 - y2))
                y1 -= gwg->GetHeight() * TR_H;
            else
                y1 += gwg->GetHeight() * TR_H;
        }



        door_point_x = (DOOR_CONSTS[GAMECLIENT.GetPlayer(player)->nation][type] * (x1 - x2)) / (y1 - y2);
    }

    return (short)(door_point_x & 0xFFFF);
}

noFlag* noBaseBuilding::GetFlag() const
{
    return gwg->GetSpecObj<noFlag>(gwg->GetNeighbour(pos, 4));
}


void noBaseBuilding::WareNotNeeded(Ware* ware)
{
    if (!ware)
    {
        std::cerr << "Warning: Trying to remove non-existing ware. Please report this replay to https://bugs.launchpad.net/s25rttr/!" << std::endl;
        return;
    }

    if(ware->LieInWarehouse())
    {
        // Bestellung im Lagerhaus stornieren
        static_cast<nobBaseWarehouse*>(ware->GetLocation())->CancelWare(ware);
        // Ware muss auch noch vernichtet werden!
        // Inventur entsprechend verringern
        //GAMECLIENT.GetPlayer(player)->DecreaseInventoryWare(ware->type,1);
        GAMECLIENT.GetPlayer(player)->RemoveWare(ware);
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
            MapPoint nb = gwg->GetNeighbour(pos, i);

            noBase* no = gwg->GetSpecObj<noBase>(nb);
            if(no)
            {
                no->Destroy();
                delete no;
                gwg->SetNO(NULL, nb);
            }
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
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 6 : 1));
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type);
}

/// Gibt ein Bild zurück für das Gebäudegerüst
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImage() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 3);
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 2);
}

/// Gibt ein Bild zurück für das normale Gebäude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImageShadow() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 2);
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 1);
}

/// Gibt ein Bild zurück für das Gebäudegerüst
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImageShadow() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 4);
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 3);
}

/// Gibt ein Bild zurück für die Tür des Gebäudes
glArchivItem_Bitmap* noBaseBuilding::GetDoorImage() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 7 : 5));
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 4);
}

