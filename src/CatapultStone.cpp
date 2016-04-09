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
#include "CatapultStone.h"

#include "EventManager.h"
#include "SerializedGameData.h"
#include "Loader.h"
#include "GameClient.h"
#include "buildings/nobMilitary.h"
#include "nodeObjs/noEnvObject.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldView.h"
#include "Random.h"
#include "gameData/MapConsts.h"

#include <cmath>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class noBase;

CatapultStone::CatapultStone(const MapPoint dest_building, const MapPoint dest_map,
                             const int start_x, const int start_y, const int dest_x, const int dest_y, const unsigned fly_duration) :
    dest_building(dest_building), dest_map(dest_map), start_x(start_x),
    start_y(start_y), dest_x(dest_x), dest_y(dest_y), explode(false)
{
    event = em->AddEvent(this, fly_duration);
}



CatapultStone::CatapultStone(SerializedGameData& sgd, const unsigned obj_id) : GameObject(sgd, obj_id),
    dest_building(sgd.PopMapPoint()),
    dest_map(sgd.PopMapPoint()),
    start_x(sgd.PopSignedInt()),
    start_y(sgd.PopSignedInt()),
    dest_x(sgd.PopSignedInt()),
    dest_y(sgd.PopSignedInt()),
    explode(sgd.PopBool()),
    event(sgd.PopObject<EventManager::Event>(GOT_EVENT))
{
}


/// Serialisierungsfunktionen
void CatapultStone::Serialize_CatapultStone(SerializedGameData& sgd) const
{
    sgd.PushMapPoint(dest_building);
    sgd.PushMapPoint(dest_map);
    sgd.PushSignedInt(start_x);
    sgd.PushSignedInt(start_y);
    sgd.PushSignedInt(dest_x);
    sgd.PushSignedInt(dest_y);
    sgd.PushBool(explode);
    sgd.PushObject(event, true);
}

void CatapultStone::Destroy()
{
}

void CatapultStone::Draw(const GameWorldView& gwv, const int xoffset, const int yoffset)
{
    // Stein überhaupt zeichnen (wenn Quelle und Ziel nicht sichtbar sind, dann nicht!)
    if(gwv.GetViewer().GetVisibility(dest_building) != VIS_VISIBLE &&
            gwv.GetViewer().GetVisibility(dest_map) != VIS_VISIBLE)
        return;

    int world_width = gwg->GetWidth() * TR_W;
    int world_height = gwg->GetHeight() * TR_H;

    if(explode)
    {
        // Stein explodierend am Ziel zeichnen
        LOADER.GetMapPlayerImage(3102 + GAMECLIENT.Interpolate(4, event))->
        Draw((dest_x - xoffset + world_width) % world_width, (dest_y - yoffset + world_height) % world_height);
    }
    else
    {
        // Linear interpolieren zwischen Ausgangs- und Zielpunkt
        int x = GAMECLIENT.Interpolate(start_x, dest_x, event);
        int y = GAMECLIENT.Interpolate(start_y, dest_y, event);

        double whole = std::sqrt(double((dest_x - start_x) * (dest_x - start_x) + (dest_y - start_y) * (dest_y - start_y)));
        int s = GAMECLIENT.Interpolate(static_cast<int>(whole), event);


        double dx = double(s) / whole  - 0.5;

        // Y-Verschiebung ausrechnen, damit die Nullpunkte beim Start- und Endpunkt liegen
        double y_diff = 0.5 * 0.5;

        // Verschiebung ausrechnen von Y
        int diff = int((dx * dx - y_diff) * 200);

        // Schatten auf linearer Linie zeichnen
        LOADER.GetMapImageN(3101)->Draw((x - xoffset + world_width) % world_width, (y - yoffset + world_height) % world_height, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
        // Stein auf Parabel zeichnen
        LOADER.GetMapPlayerImage(3100)->Draw((x - xoffset + world_width) % world_width, (y - yoffset + world_height + diff) % world_height);
    }
}

void CatapultStone::HandleEvent(const unsigned int  /*id*/)
{
    if(explode)
    {
        // Explodiert --> mich zerstören
        gwg->RemoveCatapultStone(this);
        em->AddToKillList(this);
    }
    else
    {
        // Stein ist aufgeschlagen --> Explodierevent anmelden
        event = em->AddEvent(this, 10);
        explode = true;

        // Trifft der Stein?
        if(dest_building == dest_map)
        {
            // Steht an der Stelle noch ein Militärgebäude zum Bombardieren?
            nobMilitary* milBld = gwg->GetSpecObj<nobMilitary>(dest_building);
            if(milBld)
            {
                milBld->HitOfCatapultStone();
                // If there are no troops left, destroy it
                if(milBld->troops.empty())
                    gwg->DestroyNO(milBld->GetPos());
            }
        }
        else
        {
            // Trifft nicht
            // ggf. Leiche hinlegen, falls da nix ist
            if(!gwg->GetSpecObj<noBase>(dest_map))
                gwg->SetNO(dest_map, new noEnvObject(dest_map, 502 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2)));
        }
    }
}
