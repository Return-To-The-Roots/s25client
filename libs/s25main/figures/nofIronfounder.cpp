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

#include "nofIronfounder.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorldGame.h"

nofIronfounder::nofIronfounder(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::IronFounder, pos, player, workplace)
{}

nofIronfounder::nofIronfounder(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofIronfounder::DrawWorking(DrawPoint drawPt)
{
    const helpers::EnumArray<DrawPoint, Nation> offsets = {{{-22, 12}, {-23, 3}, {-19, 8}, {-18, 4}, {-33, 7}}};

    unsigned now_id = GAMECLIENT.Interpolate(272, current_ev);

    if(now_id < 182)
    {
        LOADER.GetPlayerImage("rom_bobs", 100 + (now_id % 8))
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     gwg->GetPlayer(workplace->GetPlayer()).color);

        // Evtl Sound abspielen
        if(now_id % 16 == 3)
        {
            gwg->GetSoundMgr().playNOSound(58, *this, now_id / 16);
            was_sounding = true;
        }
    }
}

helpers::OptionalEnum<GoodType> nofIronfounder::ProduceWare()
{
    return GoodType::Iron;
}
