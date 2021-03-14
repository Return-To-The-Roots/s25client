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

#include "nofMinter.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofMinter::nofMinter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Minter, pos, player, workplace)
{}

nofMinter::nofMinter(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofMinter::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{19, -20}, {19, -11}, {22, -12}, {28, 1}, {16, -12}}};

    unsigned now_id = GAMECLIENT.Interpolate(136, current_ev);

    if(now_id < 91)
    {
        LOADER.GetPlayerImage("rom_bobs", 84 + (now_id) % 8)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            gwg->GetSoundMgr().playNOSound(58, *this, now_id);
            was_sounding = true;
        }
    }

    last_id = now_id;
}

helpers::OptionalEnum<GoodType> nofMinter::ProduceWare()
{
    world->GetPlayer(player).ChangeStatisticValue(StatisticType::Gold, 1);
    return GoodType::Coins;
}
