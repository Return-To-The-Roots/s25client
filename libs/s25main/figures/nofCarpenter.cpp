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

#include "rttrDefines.h" // IWYU pragma: keep
#include "nofCarpenter.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorldGame.h"

nofCarpenter::nofCarpenter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_CARPENTER, pos, player, workplace)
{}

nofCarpenter::nofCarpenter(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofCarpenter::DrawWorking(DrawPoint drawPt)
{
    static const DrawPointInit offsets[NUM_NATS] = {{30, 3}, {38, 3}, {30, 8}, {17, -2}, {38, 3}};

    unsigned now_id;

    LOADER.GetPlayerImage("rom_bobs", 32 + ((now_id = GAMECLIENT.Interpolate(136, current_ev)) % 8))
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, gwg->GetPlayer(workplace->GetPlayer()).color);

    // Evtl Sound abspielen
    if(now_id % 8 == 3 || now_id % 8 == 7)
    {
        SOUNDMANAGER.PlayNOSound(54 + ((now_id) % 8) / 4, this, now_id / 4);
        was_sounding = true;
    }

    last_id = now_id;
}

GoodType nofCarpenter::ProduceWare()
{
    return GD_BOARDS;
}
