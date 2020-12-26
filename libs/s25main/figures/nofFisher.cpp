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

#include "nofFisher.h"

#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"

nofFisher::nofFisher(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_FISHER, pos, player, workplace), fishing_dir(Direction::WEST), successful(false)
{}

void nofFisher::Serialize_nofFisher(SerializedGameData& sgd) const
{
    Serialize_nofFarmhand(sgd);

    sgd.PushEnum<uint8_t>(fishing_dir);
    sgd.PushBool(successful);
}

nofFisher::nofFisher(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), fishing_dir(sgd.Pop<Direction>()), successful(sgd.PopBool())
{}

/// Malt den Arbeiter beim Arbeiten
void nofFisher::DrawWorking(DrawPoint drawPt)
{
    unsigned short id = GAMECLIENT.Interpolate(232, current_ev);
    unsigned short draw_id;

    if(id < 16)
    {
        // Angel reinlassen
        const unsigned iFishingDir = rttr::enum_cast(fishing_dir);
        if(iFishingDir < 3)
            draw_id = 1566 + 8 * iFishingDir + (id / 2);
        else
            draw_id = 108 + 8 * (iFishingDir - 3) + (id / 2);

        if(id / 2 == 1)
        {
            SOUNDMANAGER.PlayNOSound(62, this, 0);
            was_sounding = true;
        }
    } else if(id < 216)
    {
        // Angel im Wasser hängen lassen und warten
        draw_id = 1590 + 8 * rttr::enum_cast(fishing_dir + 3) + (id % 8);
    } else
    {
        // Angel wieder rausholn
        if(successful)
            // Mit Fisch an der Angel
            draw_id = 1638 + 8 * rttr::enum_cast(fishing_dir + 3) + (id - 216) / 2;
        else
        {
            // Ohne Fisch (wie das Reinlassen, bloß umgekehrt)
            const unsigned iFishingDir = rttr::enum_cast(fishing_dir);
            if(iFishingDir < 3)
                draw_id = 1566 + 8 * iFishingDir + 7 - (id - 216) / 2;
            else
                draw_id = 108 + 8 * (iFishingDir - 3) + 7 - (id - 216) / 2;
        }

        if((id - 216) / 2 == 1)
        {
            SOUNDMANAGER.PlayNOSound(62, this, 1);
            was_sounding = true;
        }
    }

    LOADER.GetPlayerImage("rom_bobs", draw_id)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
    DrawShadow(drawPt, 0, Direction(fishing_dir));
}

unsigned short nofFisher::GetCarryID() const
{
    return 70;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofFisher::WorkStarted()
{
    unsigned char doffset = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);
    // Punkt mit Fisch suchen (mit zufälliger Richtung beginnen)
    for(Direction dir : helpers::EnumRange<Direction>{})
    {
        fishing_dir = dir + doffset;
        Resource neighbourRes = gwg->GetNode(gwg->GetNeighbour(pos, fishing_dir)).resources;
        if(neighbourRes.has(Resource::Fish))
            break;
    }

    // Wahrscheinlichkeit, einen Fisch zu fangen sinkt mit abnehmendem Bestand
    unsigned short probability = 40 + (gwg->GetNode(gwg->GetNeighbour(pos, fishing_dir)).resources.getAmount()) * 10;
    successful = (RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 100) < probability);
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofFisher::WorkFinished()
{
    // Wenn ich einen Fisch gefangen habe, den Fisch "abbauen" und in die Hand nehmen
    if(successful)
    {
        if(!gwg->GetGGS().isEnabled(AddonId::INEXHAUSTIBLE_FISH))
            gwg->ReduceResource(gwg->GetNeighbour(pos, fishing_dir));
        ware = GD_FISH;
    } else
        ware = boost::none;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFisher::GetPointQuality(const MapPoint pt) const
{
    // Der Punkt muss passierbar sein für Figuren
    if(!PathConditionHuman(*gwg).IsNodeOk(pt))
        return PQ_NOTPOSSIBLE;

    // irgendwo drumherum muss es Fisch geben
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(gwg->GetNode(gwg->GetNeighbour(pt, dir)).resources.has(Resource::Fish))
            return PQ_CLASS1;
    }

    return PQ_NOTPOSSIBLE;
}
