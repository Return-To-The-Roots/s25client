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

#include "nofFarmer.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrainfield.h"

nofFarmer::nofFarmer(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_FARMER, pos, player, workplace), harvest(false)
{}

void nofFarmer::Serialize_nofFarmer(SerializedGameData& sgd) const
{
    Serialize_nofFarmhand(sgd);

    sgd.PushBool(harvest);
}

nofFarmer::nofFarmer(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool()) {}

/// Malt den Arbeiter beim Arbeiten
void nofFarmer::DrawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    if(harvest)
    {
        LOADER.GetPlayerImage("rom_bobs", 140 + (now_id = GAMECLIENT.Interpolate(88, current_ev)) % 8)
          ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            SOUNDMANAGER.PlayNOSound(64, this, now_id / 8);
            was_sounding = true;
        }

    } else
    {
        LOADER.GetPlayerImage("rom_bobs", 132 + GAMECLIENT.Interpolate(88, current_ev) % 8)
          ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
    }
}

unsigned short nofFarmer::GetCarryID() const
{
    return 71;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofFarmer::WorkStarted()
{
    // Wenn ich zu einem Getreidefeld gehe, ernte ich es ab, ansonsten sähe ich
    harvest = (gwg->GetNO(pos)->GetType() == NOP_GRAINFIELD);

    // Getreidefeld Bescheid sagen, damits nicht plötzlich verschwindet, während wir arbeiten
    if(harvest)
        gwg->GetSpecObj<noGrainfield>(pos)->BeginHarvesting();
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofFarmer::WorkFinished()
{
    if(harvest)
    {
        // Getreidefeld vernichten und vorher noch die ID von dem abgeernteten Feld holen, was dann als
        // normales Zierobjekt gesetzt wird
        noBase* nob = gwg->GetNO(pos);
        // Check if there is still a grain field at this position
        if(nob->GetGOT() != GOT_GRAINFIELD)
            return;
        unsigned mapLstId = static_cast<noGrainfield*>(nob)->GetHarvestMapLstID();
        gwg->DestroyNO(pos);
        gwg->SetNO(pos, new noEnvObject(pos, mapLstId));

        // Getreide, was wir geerntet haben, in die Hand nehmen
        ware = GD_GRAIN;
    } else
    {
        // If the point got bad (e.g. something was build), abort work
        if(GetPointQuality(pos) == PQ_NOTPOSSIBLE)
            return;

        // Was stand hier vorher?
        NodalObjectType noType = gwg->GetNO(pos)->GetType();

        // Nur Zierobjekte und Schilder dürfen weggerissen werden
        if(noType == NOP_ENVIRONMENT || noType == NOP_NOTHING)
        {
            gwg->DestroyNO(pos, false);
            // neues Getreidefeld setzen
            gwg->SetNO(pos, new noGrainfield(pos));
        }

        // Wir haben nur gesäht (gar nichts in die Hand nehmen)
        ware = boost::none;
    }

    // BQ drumrum neu berechnen
    gwg->RecalcBQAroundPoint(pos);
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFarmer::GetPointQuality(const MapPoint pt) const
{
    // Entweder gibts ein Getreidefeld, das wir abernten können...
    if(gwg->GetNO(pt)->GetType() == NOP_GRAINFIELD)
    {
        if(gwg->GetSpecObj<noGrainfield>(pt)->IsHarvestable())
            return PQ_CLASS1;
        else
            return PQ_NOTPOSSIBLE;
    }
    // oder einen freien Platz, wo wir ein neues sähen können
    else
    {
        // Nicht auf Straßen bauen!
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(gwg->GetPointRoad(pt, dir) != PointRoad::None)
                return PQ_NOTPOSSIBLE;
        }

        // Terrain untersuchen
        if(!gwg->IsOfTerrain(pt, [](const auto& desc) { return desc.IsVital(); }))
            return PQ_NOTPOSSIBLE;

        // Ist Platz frei?
        NodalObjectType noType = gwg->GetNO(pt)->GetType();
        if(noType != NOP_ENVIRONMENT && noType != NOP_NOTHING)
            return PQ_NOTPOSSIBLE;

        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            // Nicht direkt neben andere Getreidefelder und Gebäude setzen!
            noType = gwg->GetNO(gwg->GetNeighbour(pt, dir))->GetType();
            if(noType == NOP_GRAINFIELD || noType == NOP_BUILDING || noType == NOP_BUILDINGSITE)
                return PQ_NOTPOSSIBLE;
        }

        return PQ_CLASS2;
    }
}

void nofFarmer::WorkAborted()
{
    nofFarmhand::WorkAborted();
    // dem Getreidefeld Bescheid sagen, damit es wieder verdorren kann, wenn wir abernten
    if(harvest && state == STATE_WORK)
        gwg->GetSpecObj<noGrainfield>(pos)->EndHarvesting();
}
