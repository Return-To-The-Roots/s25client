// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofFarmer.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrainfield.h"

nofFarmer::nofFarmer(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Farmer, pos, player, workplace), harvest(false)
{}

void nofFarmer::Serialize(SerializedGameData& sgd) const
{
    nofFarmhand::Serialize(sgd);

    sgd.PushBool(harvest);
}

nofFarmer::nofFarmer(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool())
{}

/// Malt den Arbeiter beim Arbeiten
void nofFarmer::DrawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    if(harvest)
    {
        LOADER.GetPlayerImage("rom_bobs", 140 + (now_id = GAMECLIENT.Interpolate(88, current_ev)) % 8)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            world->GetSoundMgr().playNOSound(64, *this, now_id / 8);
            was_sounding = true;
        }

    } else
    {
        LOADER.GetPlayerImage("rom_bobs", 132 + GAMECLIENT.Interpolate(88, current_ev) % 8)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
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
    harvest = (world->GetNO(pos)->GetType() == NodalObjectType::Grainfield);

    // Getreidefeld Bescheid sagen, damits nicht plötzlich verschwindet, während wir arbeiten
    if(harvest)
        world->GetSpecObj<noGrainfield>(pos)->BeginHarvesting();
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofFarmer::WorkFinished()
{
    if(harvest)
    {
        // Getreidefeld vernichten und vorher noch die ID von dem abgeernteten Feld holen, was dann als
        // normales Zierobjekt gesetzt wird
        noBase* nob = world->GetNO(pos);
        // Check if there is still a grain field at this position
        if(nob->GetGOT() != GO_Type::Grainfield)
            return;
        unsigned mapLstId = static_cast<noGrainfield*>(nob)->GetHarvestMapLstID();
        world->DestroyNO(pos);
        world->SetNO(pos, new noEnvObject(pos, mapLstId));

        // Getreide, was wir geerntet haben, in die Hand nehmen
        ware = GoodType::Grain;
    } else
    {
        // If the point got bad (e.g. something was build), abort work
        if(GetPointQuality(pos) == PointQuality::NotPossible)
            return;

        // Was stand hier vorher?
        NodalObjectType noType = world->GetNO(pos)->GetType();

        // Nur Zierobjekte und Schilder dürfen weggerissen werden
        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->DestroyNO(pos, false);
            // neues Getreidefeld setzen
            world->SetNO(pos, new noGrainfield(pos));
        }

        // Wir haben nur gesäht (gar nichts in die Hand nehmen)
        ware = boost::none;
    }

    // BQ drumrum neu berechnen
    world->RecalcBQAroundPoint(pos);
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFarmer::GetPointQuality(const MapPoint pt) const
{
    // Entweder gibts ein Getreidefeld, das wir abernten können...
    if(world->GetNO(pt)->GetType() == NodalObjectType::Grainfield)
    {
        if(world->GetSpecObj<noGrainfield>(pt)->IsHarvestable())
            return PointQuality::Class1;
        else
            return PointQuality::NotPossible;
    }
    // oder einen freien Platz, wo wir ein neues sähen können
    else
    {
        // Nicht auf Straßen bauen!
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(world->GetPointRoad(pt, dir) != PointRoad::None)
                return PointQuality::NotPossible;
        }

        // Terrain untersuchen
        if(!world->IsOfTerrain(pt, [](const auto& desc) { return desc.IsVital(); }))
            return PointQuality::NotPossible;

        // Ist Platz frei?
        NodalObjectType noType = world->GetNO(pt)->GetType();
        if(noType != NodalObjectType::Environment && noType != NodalObjectType::Nothing)
            return PointQuality::NotPossible;

        for(const MapPoint nb : world->GetNeighbours(pt))
        {
            // Nicht direkt neben andere Getreidefelder und Gebäude setzen!
            noType = world->GetNO(nb)->GetType();
            if(noType == NodalObjectType::Grainfield || noType == NodalObjectType::Grapefield
               || noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
                return PointQuality::NotPossible;
        }

        return PointQuality::Class2;
    }
}

void nofFarmer::WorkAborted()
{
    nofFarmhand::WorkAborted();
    // dem Getreidefeld Bescheid sagen, damit es wieder verdorren kann, wenn wir abernten
    if(harvest && state == State::Work)
        world->GetSpecObj<noGrainfield>(pos)->EndHarvesting();
}
