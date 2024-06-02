// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWinegrower.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "WineLoader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrapefield.h"
#include "gameData/TerrainDesc.h"
#include <stdexcept>

using namespace wineaddon;

nofWinegrower::nofWinegrower(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Winegrower, pos, player, workplace), harvest(false)
{}

nofWinegrower::nofWinegrower(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool())
{}

void nofWinegrower::Serialize(SerializedGameData& sgd) const
{
    nofFarmhand::Serialize(sgd);
    sgd.PushBool(harvest);
}

/// Malt den Arbeiter beim Arbeiten
void nofWinegrower::DrawWorking(DrawPoint drawPt)
{
    unsigned short now_id;
    if(harvest)
    {
        now_id = GAMECLIENT.Interpolate(44, current_ev);
        LOADER
          .GetPlayerImage("wine_bobs", getStartIndexOfBob(BobTypes::WINEGROWER_PICKING_GRAPES_ANIMATION) + now_id % 4)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    } else
    {
        now_id = GAMECLIENT.Interpolate(4 * 16 + 4 * 8, current_ev);
        unsigned draw_id;
        if(now_id < 64)
            draw_id = getStartIndexOfBob(BobTypes::WINEGROWER_DIGGING_ANIMATION) + now_id % 15;
        else
            draw_id = getStartIndexOfBob(BobTypes::WINEGROWER_PLANTING_ANIMATION) + now_id % 4;

        LOADER.GetPlayerImage("wine_bobs", draw_id)->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    }

    // Schaufel-Sound
    if(now_id % 8 == 3)
    {
        world->GetSoundMgr().playNOSound(76, *this, now_id, 200);
        was_sounding = true;
    }
}

unsigned short nofWinegrower::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofWinegrower::WorkStarted()
{
    // Weinberg Bescheid sagen, damits nicht plötzlich verschwindet, während wir arbeiten
    if(harvest)
        world->GetSpecObj<noGrapefield>(pos)->BeginHarvesting();
};

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofWinegrower::WorkFinished()
{
    if(harvest)
    {
        // Weinberg vernichten und vorher noch die ID von dem abgeernteten Weinberg holen, was dann als
        // normales Zierobjekt gesetzt wird
        noBase* nob = world->GetNO(pos);
        // Check if there is still a grapes field at this position
        if(nob->GetGOT() != GO_Type::Grapefield)
            return;
        unsigned wine_bobs = static_cast<noGrapefield*>(nob)->GetHarvestID();
        world->DestroyNO(pos);
        world->SetNO(pos, new noEnvObject(pos, wine_bobs, 7));

        // Getreide, was wir geerntet haben, in die Hand nehmen
        ware = GoodType::Grapes;
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
            // neuen Weinberg setzen
            world->SetNO(pos, new noGrapefield(pos));
        }

        // Wir haben nur gesäht (gar nichts in die Hand nehmen)
        ware = boost::none;
    }

    // BQ drumrum neu berechnen
    world->RecalcBQAroundPoint(pos);
}

/// Fragt abgeleitete Klasse, ob hier Platz bzw ob hier ein Baum etc steht, den z.B. der Holzfäller braucht
nofFarmhand::PointQuality nofWinegrower::GetPointQuality(const MapPoint pt) const
{
    // Entweder gibts einen Weinberg, den wir abernten können...
    if(world->GetNO(pt)->GetType() == NodalObjectType::Grapefield)
    {
        if(world->GetSpecObj<noGrapefield>(pt)->IsHarvestable())
            return PointQuality::Class1;
        else
            return PointQuality::NotPossible;
    }
    // oder einen freien Platz, wo wir einen neuen anlegen können
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
            // Nicht direkt neben anderen Weinbergen und Gebäude setzen!
            noType = world->GetNO(nb)->GetType();
            if(noType == NodalObjectType::Grapefield || noType == NodalObjectType::Building
               || noType == NodalObjectType::Buildingsite)
                return PointQuality::NotPossible;
        }

        return PointQuality::Class2;
    }
}

void nofWinegrower::WorkAborted()
{
    nofFarmhand::WorkAborted();
    // dem Weinberg Bescheid sagen, damit er wieder verdorren kann, wenn wir abernten
    if(harvest && state == State::Work)
        world->GetSpecObj<noGrapefield>(pos)->EndHarvesting();
}

/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofWinegrower::WalkingStarted()
{
    // Wenn ich zu einem Weinberg gehe, ernte ich es ab, ansonsten pflanze ich
    harvest = (world->GetNO(dest)->GetType() == NodalObjectType::Grapefield);

    if(!harvest)
        workplace->ConsumeWares();
}

/// Draws the figure while returning home / entering the building (often carrying wares)
void nofWinegrower::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "wine_bobs", getStartIndexOfBob(BobTypes::WINEGROWER_WALKING_WITH_FULL_BASKET));
}

/// Draws the winegrower while walking
/// (overriding standard method of nofFarmhand)
void nofWinegrower::DrawOtherStates(DrawPoint drawPt)
{
    switch(state)
    {
        case State::WalkToWorkpoint:
        {
            // Go to harvest grapes?
            if(harvest)
            {
                DrawWalking(drawPt, "wine_bobs", getStartIndexOfBob(BobTypes::WINEGROWER_WALKING_WITH_EMPTY_BASKET));
            } else
                // Draw normal walking
                DrawWalking(drawPt);
        }
        break;
        default: return;
    }
}
