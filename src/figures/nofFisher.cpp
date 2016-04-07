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
#include "nofFisher.h"

#include "Loader.h"
#include "GameClient.h"
#include "Random.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap_Player.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class nobUsual;

nofFisher::nofFisher(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_FISHER, pos, player, workplace), fishing_dir(0), successful(false)
{
}

void nofFisher::Serialize_nofFisher(SerializedGameData& sgd) const
{
    Serialize_nofFarmhand(sgd);

    sgd.PushUnsignedChar(fishing_dir);
    sgd.PushBool(successful);
}

nofFisher::nofFisher(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id),
    fishing_dir(sgd.PopUnsignedChar()),
    successful(sgd.PopBool())
{
}

/// Malt den Arbeiter beim Arbeiten
void nofFisher::DrawWorking(int x, int y)
{
    unsigned short id = GAMECLIENT.Interpolate(232, current_ev);
    unsigned short draw_id;

    if(id < 16)
    {
        // Angel reinlassen
        if(fishing_dir < 3)
            draw_id = 1566 + 8 * fishing_dir + (id / 2);
        else
            draw_id = 108 + 8 * (fishing_dir - 3) + (id / 2);

        if(id / 2 == 1)
        {
            SOUNDMANAGER.PlayNOSound(62, this, 0);
            was_sounding = true;
        }
    }
    else if(id < 216)
    {
        // Angel im Wasser hängen lassen und warten
        draw_id = 1590 + 8 * ((fishing_dir + 3) % 6) + (id % 8);
    }
    else
    {
        // Angel wieder rausholn
        if(successful)
            // Mit Fisch an der Angel
            draw_id = 1638 + 8 * ((fishing_dir + 3) % 6) + (id - 216) / 2;
        else
        {
            // Ohne Fisch (wie das Reinlassen, bloß umgekehrt)
            if(fishing_dir < 3)
                draw_id = 1566 + 8 * fishing_dir + 7 - +(id - 216) / 2;
            else
                draw_id = 108 + 8 * (fishing_dir - 3) + 7 - (id - 216) / 2;
        }

        if((id - 216) / 2 == 1)
        {
            SOUNDMANAGER.PlayNOSound(62, this, 1);
            was_sounding = true;
        }
    }

    LOADER.GetPlayerImage("rom_bobs", draw_id)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(player).color);
    DrawShadow(x, y, 0, fishing_dir);
}

/// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
unsigned short nofFisher::GetCarryID() const
{
    return 70;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofFisher::WorkStarted()
{
    unsigned char doffset = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);
    // Punkt mit Fisch suchen (mit zufälliger Richtung beginnen)
    fishing_dir = 0xFF;
    for(unsigned char i = 0; i < 6; ++i)
    {
        fishing_dir = (i + doffset) % 6;
        unsigned char neighbourRes = gwg->GetNode(gwg->GetNeighbour(pos, fishing_dir)).resources;
        if(neighbourRes > 0x80 && neighbourRes < 0x90)
            break;
    }

    // Wahrscheinlichkeit, einen Fisch zu fangen sinkt mit abnehmendem Bestand
    unsigned short probability = 40 + (gwg->GetNode(gwg->GetNeighbour(pos, fishing_dir)).resources - 0x80) * 10;
    successful = (RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 100) < probability);
}



/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofFisher::WorkFinished()
{
    // Wenn ich einen Fisch gefangen habe, den Fisch "abbauen" und in die Hand nehmen
    if(successful)
    {
        if(!GAMECLIENT.GetGGS().isEnabled(AddonId::INEXHAUSTIBLE_FISH))
            gwg->ReduceResource(gwg->GetNeighbour(pos, fishing_dir));
        ware = GD_FISH;
    }
    else
        ware = GD_NOTHING;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFisher::GetPointQuality(const MapPoint pt)
{
    // Der Punkt muss passierbar sein für Figuren
    if(!gwg->IsNodeForFigures(pt))
        return PQ_NOTPOSSIBLE;

    // irgendwo drumherum muss es Fisch geben
    for(unsigned char i = 0; i < 6; ++i)
    {
        if(gwg->GetNode(gwg->GetNeighbour(pt, i)).resources > 0x80 &&
                gwg->GetNode(gwg->GetNeighbour(pt, i)).resources < 0x90)
            return PQ_CLASS1;
    }

    return PQ_NOTPOSSIBLE;
}

