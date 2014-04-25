// $Id: nofFarmer.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofFarmer.h"

#include "GameWorld.h"
#include "noGranite.h"
#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "Ware.h"
#include "noEnvObject.h"
#include "noGrainfield.h"
#include "SoundManager.h"
#include "nobUsual.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofFarmer::nofFarmer(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_FARMER, x, y, player, workplace), harvest(false)
{
}


void nofFarmer::Serialize_nofFarmer(SerializedGameData* sgd) const
{
    Serialize_nofFarmhand(sgd);

    sgd->PushBool(harvest);
}

nofFarmer::nofFarmer(SerializedGameData* sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id),
    harvest(sgd->PopBool())
{
}


/// Malt den Arbeiter beim Arbeiten
void nofFarmer::DrawWorking(int x, int y)
{
    unsigned now_id;


    if(harvest)
    {
        LOADER.GetImageN("rom_bobs", 140 + (now_id = GAMECLIENT.Interpolate(88, current_ev)) % 8)
        ->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            SoundManager::inst().PlayNOSound(64, this, now_id / 8);
            was_sounding = true;
        }

    }
    else
    {
        LOADER.GetImageN("rom_bobs", 132 + GAMECLIENT.Interpolate(88, current_ev) % 8)
        ->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
    }


}

/// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
unsigned short nofFarmer::GetCarryID() const
{
    return 71;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofFarmer::WorkStarted()
{
    // Wenn ich zu einem Getreidefeld gehe, ernte ich es ab, ansonsten sähe ich
    harvest = (gwg->GetNO(x, y)->GetType() == NOP_GRAINFIELD);

    // Getreidefeld Bescheid sagen, damits nicht plötzlich verschwindet, während wir arbeiten
    if(harvest)
        gwg->GetSpecObj<noGrainfield>(x, y)->BeginHarvesting();
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofFarmer::WorkFinished()
{
    if(harvest)
    {
        // Getreidefeld vernichten und vorher noch die ID von dem abgeernteten Feld holen, was dann als
        // normales Zierobjekt gesetzt wird
        noBase* nob = gwg->GetNO(x, y);
        // Check if there is still a grain field at this position
        if(nob->GetGOT() != GOT_GRAINFIELD)
            return;
        noGrainfield* gf = static_cast<noGrainfield*>(nob);
        //unsigned env_obj_id = gf->GetHarvestMapLstID();
        gwg->SetNO(new noEnvObject(x, y, gf->GetHarvestMapLstID()), x, y);
        gf->Destroy();
        delete gf;

        // Getreide, was wir geerntet haben, in die Hand nehmen
        ware = GD_GRAIN;
    }
    else
    {
        // If there is any road now, don't set the grain field
        for(unsigned i = 0; i < 6; ++i)
        {
            if(gwg->GetPointRoad(x, y, i))
                return;
        }

        // Was stand hier vorher?
        NodalObjectType nop = gwg->GetNO(x, y)->GetType();

        // Nur Zierobjekte und Schilder dürfen weggerissen werden
        if(nop == NOP_ENVIRONMENT || nop == NOP_NOTHING)
        {
            // ggf. vorher wegreißen
            noBase* no = gwg->GetSpecObj<noBase>(x, y);
            if(no)
            {
                no->Destroy();
                delete no;
            }
            // neues Getreidefeld setzen
            gwg->SetNO(new noGrainfield(x, y), x, y);
        }

        // Wir haben nur gesäht (gar nichts in die Hand nehmen)
        ware = GD_NOTHING;
    }

    // BQ drumrum neu berechnen
    gwg->RecalcBQAroundPoint(x, y);
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFarmer::GetPointQuality(const MapCoord x, const MapCoord y)
{

    // Entweder gibts ein Getreidefeld, das wir abernten können...
    if(gwg->GetNO(x, y)->GetType() == NOP_GRAINFIELD)
    {
        if(gwg->GetSpecObj<noGrainfield>(x, y)->IsHarvestable())
            return PQ_CLASS1;
        else
            return PQ_NOTPOSSIBLE;
    }
    // oder einen freien Platz, wo wir ein neues sähen können
    else
    {
        // Nicht auf Straßen bauen!
        for(unsigned char i = 0; i < 6; ++i)
        {
            if(gwg->GetPointRoad(x, y, i))
                return PQ_NOTPOSSIBLE;
        }

        // Terrain untersuchen (nur auf Wiesen und Savanne und Steppe pflanzen
        unsigned char t, good_terrains = 0;
        for(unsigned char i = 0; i < 6; ++i)
        {
            t = gwg->GetTerrainAround(x, y, i);
            if(t == 3 || (t >= 8 && t <= 12))
                ++good_terrains;
        }
        if (good_terrains != 6)
            return PQ_NOTPOSSIBLE;

        // Ist Platz frei?
        NodalObjectType nop = gwg->GetNO(x, y)->GetType();
        if(nop != NOP_ENVIRONMENT && nop && nop != NOP_NOTHING)
            return PQ_NOTPOSSIBLE;

        for(unsigned char i = 0; i < 6; ++i)
        {
            // Nicht direkt neben andere Getreidefelder und Gebäude setzen!
            nop = gwg->GetNO(gwg->GetXA(x, y, i), gwg->GetYA(x, y, i))->GetType();
            if(nop == NOP_GRAINFIELD || nop == NOP_BUILDING || nop == NOP_BUILDINGSITE)
                return PQ_NOTPOSSIBLE;
        }

        // Nicht direkt neben den Bauernhof pflanzen!
        if(x == workplace->GetX() + 1 && y == workplace->GetY())
            return PQ_NOTPOSSIBLE;

        return PQ_CLASS2;
    }

}


void nofFarmer::WorkAborted_Farmhand()
{
    // dem Getreidefeld Bescheid sagen, damit es wieder verdorren kann, wenn wir abernten
    if(harvest && state == STATE_WORK)
        gwg->GetSpecObj<noGrainfield>(x, y)->EndHarvesting();
}

