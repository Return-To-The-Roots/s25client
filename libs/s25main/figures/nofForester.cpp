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
#include "world/GameWorldGame.h"
#include "nodeObjs/noTree.h"

nofForester::nofForester(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_FORESTER, pos, player, workplace)
{}

nofForester::nofForester(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Malt den Arbeiter beim Arbeiten
void nofForester::DrawWorking(DrawPoint drawPt)
{
    unsigned short now_id = GAMECLIENT.Interpolate(36, current_ev);
    // Baum pflanzen
    LOADER.GetPlayerImage("rom_bobs", 48 + now_id)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);

    // Schaufel-Sound
    if(now_id == 7 || now_id == 18)
    {
        SOUNDMANAGER.PlayNOSound(76, this, (now_id == 7) ? 0 : 1, 200);
        was_sounding = true;
    }
    // Baum-Einpflanz-Sound
    else if(now_id == 35)
    {
        SOUNDMANAGER.PlayNOSound(57, this, 2);
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
    for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
    {
        if(gwg->GetPointRoad(pos, Direction::fromInt(dir)))
            return;
    }

    NodalObjectType noType = gwg->GetNO(pos)->GetType();
    // Wenn Objekt ein Zierobjekt ist, dann löschen, ansonsten den Baum NICHT einsetzen!
    if(noType == NOP_ENVIRONMENT || noType == NOP_NOTHING)
    {
        gwg->DestroyNO(pos, false);

        // Je nach Landschaft andere Bäume pflanzbar!
        const std::array<unsigned char, 3> NUM_AVAILABLE_TREES = {6, 3, 4};
        const helpers::MultiArray<unsigned char, 3, 6> AVAILABLE_TREES = {
          {{0, 1, 2, 6, 7, 8}, {0, 1, 7, 0xFF, 0xFF, 0xFF}, {0, 1, 6, 8, 0xFF, 0xFF}}};
        uint8_t landscapeType = std::min<uint8_t>(gwg->GetLandscapeType().value, 2);

        // jungen Baum einsetzen
        gwg->SetNO(
          pos, new noTree(
                 pos, AVAILABLE_TREES[landscapeType][RANDOM.Rand(__FILE__, __LINE__, GetObjId(), NUM_AVAILABLE_TREES[landscapeType])], 0));

        // BQ drumherum neu berechnen
        gwg->RecalcBQAroundPoint(pos);

        // Minimap Bescheid geben (neuer Baum)
        if(gwg->GetGameInterface())
            gwg->GetGameInterface()->GI_UpdateMinimap(pos);
    }
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofForester::GetPointQuality(const MapPoint pt) const
{
    // Der Platz muss frei sein
    BlockingManner bm = gwg->GetNO(pt)->GetBM();

    if(bm != BlockingManner::None)
        return PQ_NOTPOSSIBLE;

    // Kein Grenzstein darf da stehen
    if(gwg->GetNode(pt).boundary_stones[0])
        return PQ_NOTPOSSIBLE;

    // darf außerdem nich auf einer Straße liegen
    for(unsigned char dir = 0; dir < Direction::COUNT; ++dir)
    {
        if(gwg->GetPointRoad(pt, Direction::fromInt(dir)))
            return PQ_NOTPOSSIBLE;
    }

    // es dürfen außerdem keine Gebäude rund um den Baum stehen
    for(unsigned char dir = 0; dir < Direction::COUNT; ++dir)
    {
        if(gwg->GetNO(gwg->GetNeighbour(pt, Direction::fromInt(dir)))->GetType() == NOP_BUILDING)
            return PQ_NOTPOSSIBLE;
    }

    // Terrain untersuchen
    if(gwg->IsOfTerrain(pt, [](const auto& desc) { return desc.IsVital(); }))
        return PQ_CLASS1;
    else
        return PQ_NOTPOSSIBLE;
}
