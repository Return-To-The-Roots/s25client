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

#include "nofForester.h"

#include "GameInterface.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noTree.h"
#include <boost/container/static_vector.hpp>

nofForester::nofForester(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Forester, pos, player, workplace)
{}

nofForester::nofForester(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Malt den Arbeiter beim Arbeiten
void nofForester::DrawWorking(DrawPoint drawPt)
{
    unsigned short now_id = GAMECLIENT.Interpolate(36, current_ev);
    // Baum pflanzen
    LOADER.GetPlayerImage("rom_bobs", 48 + now_id)->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);

    // Schaufel-Sound
    if(now_id == 7 || now_id == 18)
    {
        world->GetSoundMgr().playNOSound(76, *this, (now_id == 7) ? 0 : 1, 200);
        was_sounding = true;
    }
    // Baum-Einpflanz-Sound
    else if(now_id == 35)
    {
        world->GetSoundMgr().playNOSound(57, *this, 2);
        was_sounding = true;
    }
}

unsigned short nofForester::GetCarryID() const
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
        if(world->GetPointRoad(pos, dir) != PointRoad::None)
            return;
    }

    NodalObjectType noType = world->GetNO(pos)->GetType();
    // Wenn Objekt ein Zierobjekt ist, dann löschen, ansonsten den Baum NICHT einsetzen!
    if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
    {
        world->DestroyNO(pos, false);

        // Je nach Landschaft andere Bäume pflanzbar!
        const std::array<boost::container::static_vector<unsigned char, 6>, 3> AVAILABLE_TREES = {
          {{0, 1, 2, 6, 7, 8}, {0, 1, 7}, {0, 1, 6, 8}}};
        uint8_t landscapeType = std::min<uint8_t>(world->GetLandscapeType().value, AVAILABLE_TREES.size() - 1);

        // jungen Baum einsetzen
        world->SetNO(pos, new noTree(pos, RANDOM_ELEMENT(AVAILABLE_TREES[landscapeType]), 0));

        // BQ drumherum neu berechnen
        world->RecalcBQAroundPoint(pos);

        // Minimap Bescheid geben (neuer Baum)
        if(world->GetGameInterface())
            world->GetGameInterface()->GI_UpdateMinimap(pos);
    }
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofForester::GetPointQuality(const MapPoint pt) const
{
    // Der Platz muss frei sein
    BlockingManner bm = world->GetNO(pt)->GetBM();

    if(bm != BlockingManner::None)
        return PointQuality::NotPossible;

    // Kein Grenzstein darf da stehen
    if(world->GetNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return PointQuality::NotPossible;

    // darf außerdem nich auf einer Straße liegen
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(world->GetPointRoad(pt, dir) != PointRoad::None)
            return PointQuality::NotPossible;
    }

    // es dürfen außerdem keine Gebäude rund um den Baum stehen
    for(const MapPoint nb : world->GetNeighbours(pt))
    {
        if(world->GetNO(nb)->GetType() == NodalObjectType::Building)
            return PointQuality::NotPossible;
    }

    // Terrain untersuchen
    if(world->IsOfTerrain(pt, [](const auto& desc) { return desc.IsVital(); }))
        return PointQuality::Class1;
    else
        return PointQuality::NotPossible;
}
