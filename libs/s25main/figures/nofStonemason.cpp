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

#include "nofStonemason.h"

#include "GameInterface.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noGranite.h"

nofStonemason::nofStonemason(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Stonemason, pos, player, workplace)
{}

nofStonemason::nofStonemason(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Malt den Arbeiter beim Arbeiten
void nofStonemason::DrawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    // Stein hauen
    LOADER.GetPlayerImage("rom_bobs", 40 + (now_id = GAMECLIENT.Interpolate(64, current_ev)) % 8)
      ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);

    if(now_id % 8 == 5)
    {
        SOUNDMANAGER.PlayNOSound(56, this, now_id);
        was_sounding = true;
    }
}

unsigned short nofStonemason::GetCarryID() const
{
    return 63;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofStonemason::WorkStarted() {}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofStonemason::WorkFinished()
{
    // Stein abhauen (wenn er nur noch ganz klein ist, dann wird er von der Landkarte getilgt)
    if(gwg->GetSpecObj<noGranite>(pos)->IsSmall())
    {
        // Granitklötzchen löschen
        gwg->DestroyNO(pos);

        // Minimap Bescheid geben (Granitglötzchen muss weg)
        if(gwg->GetGameInterface())
            gwg->GetGameInterface()->GI_UpdateMinimap(pos);

        // Drumherum BQ neu berechnen, da diese sich ja jetzt hätten ändern können
        gwg->RecalcBQAroundPoint(pos);
    } else
        // ansonsten wird er um 1 kleiner
        gwg->GetSpecObj<noGranite>(pos)->Hew();

    // Stein in die Hand nehmen
    ware = GoodType::Stones;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofStonemason::GetPointQuality(const MapPoint pt) const
{
    // An dieser Position muss es nur Stein geben
    return ((gwg->GetNO(pt)->GetType() == NodalObjectType::Granite) ? PQ_CLASS1 : PQ_NOTPOSSIBLE);
}
