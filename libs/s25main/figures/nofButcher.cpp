// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofButcher.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofButcher::nofButcher(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Butcher, pos, player, workplace)
{}

nofButcher::nofButcher(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofButcher::DrawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{38, 2}, {-3, 5}, {21, -1}, {26, -5}, {-7, 2}}};

    unsigned now_id;

    LOADER.GetPlayerImage("rom_bobs", 160 + (now_id = GAMECLIENT.Interpolate(136, current_ev)) % 6)
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);

    if(now_id % 6 == 5)
    {
        gwg->GetSoundMgr().playNOSound(66, *this, now_id / 6);
        was_sounding = true;
    }

    last_id = now_id;
}

helpers::OptionalEnum<GoodType> nofButcher::ProduceWare()
{
    return GoodType::Meat;
}
