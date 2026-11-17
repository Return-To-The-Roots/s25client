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

void nofFarmer::serialize(SerializedGameData& sgd) const
{
    nofFarmhand::serialize(sgd);

    sgd.pushBool(harvest);
}

nofFarmer::nofFarmer(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id), harvest(sgd.popBool())
{}

/// Malt den Arbeiter beim Arbeiten
void nofFarmer::drawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    if(harvest)
    {
        LOADER.getPlayerImage("rom_bobs", 140 + (now_id = GAMECLIENT.interpolate(88, current_ev)) % 8)
          ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            world->getSoundMgr().playNOSound(64, *this, now_id / 8);
            was_sounding = true;
        }

    } else
    {
        LOADER.getPlayerImage("rom_bobs", 132 + GAMECLIENT.interpolate(88, current_ev) % 8)
          ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
    }
}

unsigned short nofFarmer::getCarryID() const
{
    return 71;
}

nofFarmhand::PointQuality nofFarmer::getNewFieldPointQuality(const GameWorld& world, const MapPoint pt)
{
    // Nicht auf Straßen bauen!
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(world.getPointRoad(pt, dir) != PointRoad::None)
            return PointQuality::NotPossible;
    }

    // Terrain untersuchen
    if(!world.isOfTerrain(pt, [](const auto& desc) { return desc.isVital(); }))
        return PointQuality::NotPossible;

    // Ist Platz frei?
    NodalObjectType noType = world.getNO(pt)->getType();
    if(noType != NodalObjectType::Environment && noType != NodalObjectType::Nothing)
        return PointQuality::NotPossible;

    for(const MapPoint nb : world.getNeighbours(pt))
    {
        // Nicht direkt neben andere Getreidefelder und Gebäude setzen!
        noType = world.getNO(nb)->getType();
        if(noType == NodalObjectType::Grainfield || noType == NodalObjectType::Grapefield
           || noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
            return PointQuality::NotPossible;
    }

    return PointQuality::Class2;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofFarmer::workStarted()
{
    // Wenn ich zu einem Getreidefeld gehe, ernte ich es ab, ansonsten sähe ich
    harvest = (world->getNO(pos)->getType() == NodalObjectType::Grainfield);

    // Getreidefeld Bescheid sagen, damits nicht plötzlich verschwindet, während wir arbeiten
    if(harvest)
        world->getSpecObj<noGrainfield>(pos)->beginHarvesting();
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofFarmer::workFinished()
{
    if(harvest)
    {
        // Getreidefeld vernichten und vorher noch die ID von dem abgeernteten Feld holen, was dann als
        // normales Zierobjekt gesetzt wird
        noBase* nob = world->getNO(pos);
        // Check if there is still a grain field at this position
        if(nob->getGOT() != GO_Type::Grainfield)
            return;
        unsigned mapLstId = static_cast<noGrainfield*>(nob)->getHarvestMapLstID();
        world->destroyNO(pos);
        world->setNO(pos, new noEnvObject(pos, mapLstId));

        // Getreide, was wir geerntet haben, in die Hand nehmen
        ware = GoodType::Grain;
    } else
    {
        // If the point got bad (e.g. something was build), abort work
        if(getPointQuality(pos) == PointQuality::NotPossible)
            return;

        // Was stand hier vorher?
        NodalObjectType noType = world->getNO(pos)->getType();

        // Nur Zierobjekte und Schilder dürfen weggerissen werden
        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->destroyNO(pos, false);
            // neues Getreidefeld setzen
            world->setNO(pos, new noGrainfield(pos));
        }

        // Wir haben nur gesäht (gar nichts in die Hand nehmen)
        ware = std::nullopt;
    }

    // BQ drumrum neu berechnen
    world->recalcBQAroundPoint(pos);
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFarmer::getPointQuality(const MapPoint pt, bool /* isBeforeWork */) const
{
    // Entweder gibts ein Getreidefeld, das wir abernten können...
    if(world->getNO(pt)->getType() == NodalObjectType::Grainfield)
    {
        if(world->getSpecObj<noGrainfield>(pt)->isHarvestable())
            return PointQuality::Class1;
        else
            return PointQuality::NotPossible;
    }
    // oder einen freien Platz, wo wir ein neues sähen können
    else
        return getNewFieldPointQuality(*world, pt);
}

void nofFarmer::workAborted()
{
    nofFarmhand::workAborted();
    // dem Getreidefeld Bescheid sagen, damit es wieder verdorren kann, wenn wir abernten
    if(harvest && state == State::Work)
        world->getSpecObj<noGrainfield>(pos)->endHarvesting();
}
