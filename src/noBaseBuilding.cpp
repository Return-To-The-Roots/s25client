// $Id: noBaseBuilding.cpp 9402 2014-05-10 06:54:13Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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


#include "main.h"
#include "noBaseBuilding.h"
#include "GameWorld.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "DoorConsts.h"
#include "noExtension.h"
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

noBaseBuilding::noBaseBuilding(const NodalObjectType nop, const BuildingType type, const unsigned short x, const unsigned short y, const unsigned char player)
    : noRoadNode(nop, x, y, player), type(type), nation(GAMECLIENT.GetPlayer(player)->nation), door_point_x(1000000), door_point_y(DOOR_CONSTS[GAMECLIENT.GetPlayer(player)->nation][type])
{

    // Evtl Flagge setzen, wenn noch keine da ist
    if(gwg->GetNO(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4))->GetType() != NOP_FLAG)
    {
        // ggf. vorherige Objekte lˆschen
        noBase* no = gwg->GetSpecObj<noBase>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4));
        if(no)
        {
            no->Destroy();
            delete no;
        }
        gwg->SetNO(new noFlag(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4), player), gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4));
    }

    // Straﬂeneingang setzen (wenn nicht schon vorhanden z.b. durch vorherige Baustelle!)
    if(!gwg->GetPointRoad(x, y, 4))
    {
        gwg->SetPointRoad(x, y, 4, 1);

        // Straﬂenverbindung erstellen zwischen Flagge und Haus
        // immer von Flagge ZU Geb‰ude (!)
        std::vector<unsigned char> route(1, 1);
        // Straﬂe zuweisen
        gwg->GetSpecObj<noRoadNode>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4))->routes[1] = // der Flagge
            routes[4] = // dem Geb‰ude
                new RoadSegment(RoadSegment::RT_NORMAL, gwg->GetSpecObj<noRoadNode>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4)), this, route);
    }
    else
    {
        // vorhandene Straﬂe der Flagge nutzen
        noFlag* flag = gwg->GetSpecObj<noFlag>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4));

        assert(flag->routes[1]);
        routes[4] = flag->routes[1];
        routes[4]->SetF2(this);

    }

    // Werde/Bin ich (mal) ein groﬂes Schloss? Dann m¸ssen die Anbauten gesetzt werden
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            MapCoord xa = gwg->GetXA(x, y, i);
            MapCoord ya = gwg->GetYA(x, y, i);

            noBase* no = gwg->GetSpecObj<noBase>(xa, ya);
            if(no)
            {
                no->Destroy();
                delete no;
            }
            gwg->SetNO(new noExtension(this), xa, ya);
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
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingDestroyed, x, y, type), player);

    if(gwg->GetGameInterface())
        gwg->GetGameInterface()->GI_UpdateMinimap(x, y);

    // evtl Anbauten wieder abreiﬂen
    DestroyBuildingExtensions();

    // ggf. Fenster schlieﬂen
    gwg->ImportantObjectDestroyed(x, y);

    // Baukosten zur¸ckerstatten (nicht bei Baustellen)
    if( (GetGOT() != GOT_BUILDINGSITE) &&
            ( GameClient::inst().GetGGS().isEnabled(ADDON_REFUND_MATERIALS) ||
              GameClient::inst().GetGGS().isEnabled(ADDON_REFUND_ON_EMERGENCY) ) )
    {
        // lebt unsere Flagge noch?
        noFlag* flag = GetFlag();
        if(flag)
        {
            unsigned int percent_index = 0;

            // wenn R¸ckerstattung aktiv ist, entsprechende Prozentzahl w‰hlen
            if(GameClient::inst().GetGGS().isEnabled(ADDON_REFUND_MATERIALS))
                percent_index = GameClient::inst().GetGGS().getSelection(ADDON_REFUND_MATERIALS);

            // wenn R¸ckerstattung bei Notprogramm aktiv ist, 50% zur¸ckerstatten
            else if(gwg->GetPlayer(player)->hasEmergency() && GameClient::inst().GetGGS().isEnabled(ADDON_REFUND_ON_EMERGENCY))
                percent_index = 2;

            // wieviel kriegt man von jeder Ware wieder?
            const unsigned int percents[5] = { 0, 25, 50, 75, 100 };
            const unsigned int percent = 10 * percents[percent_index];

            // zur¸ckgaben berechnen (abgerundet)
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
                    // Abnehmer f¸r Ware finden
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
    // Geb‰ude gehen, woa uﬂerdem der Bauarbeiter baut und wo die Waren liegen

    // Door-Point noch nicht ausgerechnet?
    if(door_point_x == 1000000)
    {
        int x1 = static_cast<int>(gwg->GetTerrainX(this->x, this->y));
        int y1 = static_cast<int>(gwg->GetTerrainY(this->x, this->y));
        int x2 = static_cast<int>(gwg->GetTerrainX(gwg->GetXA(this->x, this->y, 4), gwg->GetYA(this->x, this->y, 4)));
        int y2 = static_cast<int>(gwg->GetTerrainY(gwg->GetXA(this->x, this->y, 4), gwg->GetYA(this->x, this->y, 4)));

        // Gehen wir ¸ber einen Kartenrand (horizontale Richung?)
        if(abs(x1 - x2) >= gwg->GetWidth() * TR_W / 2)
        {
            if(abs(x1 - int(gwg->GetWidth())*TR_W - x2) < abs(x1 - x2))
                x1 -= gwg->GetWidth() * TR_W;
            else
                x1 += gwg->GetWidth() * TR_W;
        }
        // Und dasselbe f¸r vertikale Richtung
        if(abs(y1 - y2) >= gwg->GetHeight() * TR_H / 2)
        {
            if(abs(y1 - int(gwg->GetHeight())*TR_H - y2) < abs(y1 - y2))
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
    return gwg->GetSpecObj<noFlag>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4));
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
    // Nur bei groﬂen Geb‰uden gibts diese Anbauten
    if(GetSize() == BQ_CASTLE || GetSize() == BQ_HARBOR)
    {

        for(unsigned i = 0; i < 3; ++i)
        {
            MapCoord xa = gwg->GetXA(x, y, i);
            MapCoord ya = gwg->GetYA(x, y, i);

            noBase* no = gwg->GetSpecObj<noBase>(xa, ya);
            if(no)
            {
                no->Destroy();
                delete no;
                gwg->SetNO(NULL, xa, ya);
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

/// Gibt ein Bild zur¸ck f¸r das normale Geb‰ude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImage() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 6 : 1));
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type);
}

/// Gibt ein Bild zur¸ck f¸r das Geb‰udeger¸st
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImage() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 3);
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 2);
}

/// Gibt ein Bild zur¸ck f¸r das normale Geb‰ude
glArchivItem_Bitmap* noBaseBuilding::GetBuildingImageShadow() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 2);
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 1);
}

/// Gibt ein Bild zur¸ck f¸r das Geb‰udeger¸st
glArchivItem_Bitmap* noBaseBuilding::GetBuildingSkeletonImageShadow() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + 4);
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 3);
}

/// Gibt ein Bild zur¸ck f¸r die T¸r des Geb‰udes
glArchivItem_Bitmap* noBaseBuilding::GetDoorImage() const
{
    if (type == BLD_CHARBURNER)
        return LOADER.GetImageN("charburner", nation * 8 + ((gwg->GetLandscapeType() == LT_WINTERWORLD) ? 7 : 5));
    else
        return LOADER.GetNationImageN(nation, 250 + 5 * type + 4);
}

