// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofForester.h"

#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "buildings/noBaseBuilding.h"
#include "network/GameClient.h"
#include "nofFarmer.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noTree.h"
#include <boost/container/static_vector.hpp>

namespace {
bool IsPotentialNewFieldForOwnFarm(GameWorld& world, const MapPoint pt, const unsigned char player)
{
    if(nofFarmer::getNewFieldPointQuality(world, pt) == nofFarmhand::PointQuality::NotPossible)
        return false;

    return world.CheckPointsInRadius(
      pt, nofFarmhand::getWorkRadius(world.getGGS(), Job::Farmer),
      [&world, player](const MapPoint farmPt, unsigned) {
          if(world.getNO(farmPt)->getType() != NodalObjectType::Building)
              return false;

          const auto* building = world.getSpecObj<noBaseBuilding>(farmPt);
          return building && building->getPlayer() == player && building->getBuildingType() == BuildingType::Farm;
      },
      false);
}
} // namespace

nofForester::nofForester(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Forester, pos, player, workplace)
{}

nofForester::nofForester(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Malt den Arbeiter beim Arbeiten
void nofForester::DrawWorking(DrawPoint drawPt)
{
    unsigned short now_id = GAMECLIENT.Interpolate(36, current_ev);
    // Baum pflanzen
    LOADER.getPlayerImage("rom_bobs", 48 + now_id)->DrawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);

    // Schaufel-Sound
    if(now_id == 7 || now_id == 18)
    {
        world->getSoundMgr().playNOSound(76, *this, (now_id == 7) ? 0 : 1, 200);
        was_sounding = true;
    }
    // Baum-Einpflanz-Sound
    else if(now_id == 35)
    {
        world->getSoundMgr().playNOSound(57, *this, 2);
        was_sounding = true;
    }
}

unsigned short nofForester::getCarryID() const
{
    return 0;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofForester::WorkStarted() {}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofForester::WorkFinished()
{
    // Wenn irgendwo ne Straße schon ist, NICHT einsetzen!
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(world->getPointRoad(pos, dir) != PointRoad::None)
            return;
    }

    NodalObjectType noType = world->getNO(pos)->getType();
    // Wenn Objekt ein Zierobjekt ist, dann löschen, ansonsten den Baum NICHT einsetzen!
    if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
    {
        world->DestroyNO(pos, false);

        // Je nach Landschaft andere Bäume pflanzbar!
        const std::array<boost::container::static_vector<unsigned char, 6>, 3> AVAILABLE_TREES = {
          {{0, 1, 2, 6, 7, 8}, {0, 1, 7}, {0, 1, 6, 8}}};
        uint8_t landscapeType = std::min<uint8_t>(world->GetLandscapeType().value, AVAILABLE_TREES.size() - 1);

        // jungen Baum einsetzen
        world->setNO(pos, new noTree(pos, RANDOM_ELEMENT(AVAILABLE_TREES[landscapeType]), 0));

        // BQ drumherum neu berechnen
        world->RecalcBQAroundPoint(pos);

        // Minimap Bescheid geben (neuer Baum)
        if(world->getGameInterface())
            world->getGameInterface()->GI_UpdateMinimap(pos);
    }
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofForester::getPointQuality(const MapPoint pt, bool /* isBeforeWork */) const
{
    // Der Platz muss frei sein
    BlockingManner bm = world->getNO(pt)->getBM();

    if(bm != BlockingManner::None)
        return PointQuality::NotPossible;

    // Kein Grenzstein darf da stehen
    if(world->getNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return PointQuality::NotPossible;

    // darf außerdem nich auf einer Straße liegen
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(world->getPointRoad(pt, dir) != PointRoad::None)
            return PointQuality::NotPossible;
    }

    // es dürfen außerdem keine Gebäude rund um den Baum stehen
    for(const MapPoint nb : world->getNeighbours(pt))
    {
        if(world->getNO(nb)->getType() == NodalObjectType::Building)
            return PointQuality::NotPossible;
    }

    if(world->getGGS().isEnabled(AddonId::FORESTER_FARM_FIELD_AVOIDANCE)
       && IsPotentialNewFieldForOwnFarm(*world, pt, player))
        return PointQuality::NotPossible;

    // Terrain untersuchen
    if(world->isOfTerrain(pt, [](const auto& desc) { return desc.isVital(); }))
        return PointQuality::Class1;
    else
        return PointQuality::NotPossible;
}
