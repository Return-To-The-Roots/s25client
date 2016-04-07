// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "nofForester.h"

#include "Loader.h"
#include "GameClient.h"
#include "Random.h"
#include "nodeObjs/noTree.h"
#include "SoundManager.h"
#include "GameInterface.h"
#include "gameData/TerrainData.h"
#include "ogl/glArchivItem_Bitmap_Player.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class SerializedGameData;
class nobUsual;

nofForester::nofForester(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_FORESTER, pos, player, workplace)
{
}

nofForester::nofForester(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id)
{
}

/// Malt den Arbeiter beim Arbeiten
void nofForester::DrawWorking(int x, int y)
{
    unsigned short now_id;
    // Baum pflanzen
    LOADER.GetPlayerImage("rom_bobs", 48 + (now_id = GAMECLIENT.Interpolate(36, current_ev)))
    ->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(player).color);

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

/// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
unsigned short nofForester::GetCarryID() const
{
    return 0;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofForester::WorkStarted()
{
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofForester::WorkFinished()
{
    // Wenn irgendwo ne Straße schon ist, NICHT einsetzen!
    for(unsigned i = 0; i < 6; ++i)
    {
        if(gwg->GetPointRoad(pos, i))
            return;
    }

    NodalObjectType noType = gwg->GetNO(pos)->GetType();
    // Wenn Objekt ein Zierobjekt ist, dann löschen, ansonsten den Baum NICHT einsetzen!
    if(noType == NOP_ENVIRONMENT || noType == NOP_NOTHING)
    {
        gwg->DestroyNO(pos, false);

        // Je nach Landschaft andere Bäume pflanzbar!
        const unsigned char AVAILABLE_TREES_COUNT[3] =
        {
            6, 3, 4
        };
        const unsigned char AVAILABLE_TREES[3][8] =
        {
            {0, 1, 2, 6, 7, 8,   0xFF, 0xFF},
            {0, 1, 7,         0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
            {0, 1, 6, 8,       0xFF, 0xFF, 0xFF, 0xFF}
        };

        // jungen Baum einsetzen
        gwg->SetNO(pos, new noTree(pos, AVAILABLE_TREES[gwg->GetLandscapeType()]
                              [RANDOM.Rand(__FILE__, __LINE__, GetObjId(), AVAILABLE_TREES_COUNT[gwg->GetLandscapeType()])], 0));

        // BQ drumherum neu berechnen
        gwg->RecalcBQAroundPoint(pos);

        // Minimap Bescheid geben (neuer Baum)
        if(gwg->GetGameInterface())
            gwg->GetGameInterface()->GI_UpdateMinimap(pos);
    }
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofForester::GetPointQuality(const MapPoint pt)
{
    // Der Platz muss frei sein
    noBase::BlockingManner bm = gwg->GetNO(pt)->GetBM();

    if(bm != noBase::BM_NOTBLOCKING)
        return PQ_NOTPOSSIBLE;

    // Kein Grenzstein darf da stehen
    if(gwg->GetNode(pt).boundary_stones[0])
        return PQ_NOTPOSSIBLE;


    // darf außerdem nich auf einer Straße liegen
    for(unsigned char i = 0; i < 6; ++i)
    {
        if(gwg->GetPointRoad(pt, i))
            return PQ_NOTPOSSIBLE;
    }

    // es dürfen außerdem keine Gebäude rund um den Baum stehen
    for(unsigned char i = 0; i < 6; ++i)
    {
        if(gwg->GetNO(gwg->GetNeighbour(pt, i))->GetType() ==  NOP_BUILDING)
            return PQ_NOTPOSSIBLE;
    }

    // Terrain untersuchen (nur auf Wiesen und Savanne und Steppe pflanzen
    unsigned char good_terrains = 0;

    for(unsigned char i = 0; i < 6; ++i)
    {
        if(TerrainData::IsVital(gwg->GetTerrainAround(pt, i)))
            ++good_terrains;
    }
    if(good_terrains != 6)
        return PQ_NOTPOSSIBLE;


    return PQ_CLASS1;
}
