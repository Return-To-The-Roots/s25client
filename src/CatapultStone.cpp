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
#include "CatapultStone.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noEnvObject.h"
#include "gameData/MapConsts.h"

#include <cmath>

CatapultStone::CatapultStone(const MapPoint dest_building, const MapPoint dest_map, const DrawPoint start, const DrawPoint dest,
                             const unsigned fly_duration)
    : dest_building(dest_building), dest_map(dest_map), startPos(start), destPos(dest), explode(false)
{
    event = GetEvMgr().AddEvent(this, fly_duration);
}

CatapultStone::CatapultStone(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), dest_building(sgd.PopMapPoint()), dest_map(sgd.PopMapPoint()), startPos(sgd.PopPoint<int>()),
      destPos(sgd.PopPoint<int>()), explode(sgd.PopBool()), event(sgd.PopEvent())
{}

/// Serialisierungsfunktionen
void CatapultStone::Serialize_CatapultStone(SerializedGameData& sgd) const
{
    sgd.PushMapPoint(dest_building);
    sgd.PushMapPoint(dest_map);
    sgd.PushPoint<int>(startPos);
    sgd.PushPoint<int>(destPos);
    sgd.PushBool(explode);
    sgd.PushEvent(event);
}

void CatapultStone::Destroy() {}

void CatapultStone::Draw(DrawPoint drawOffset)
{
    const DrawPoint worldSize = DrawPoint(gwg->GetWidth() * TR_W, gwg->GetHeight() * TR_H);

    if(explode)
    {
        // Stein explodierend am Ziel zeichnen
        DrawPoint drawPos = destPos - drawOffset + worldSize;
        drawPos.x %= worldSize.x;
        drawPos.y %= worldSize.y;
        LOADER.GetMapPlayerImage(3102 + GAMECLIENT.Interpolate(4, event))->DrawFull(drawPos);
    } else
    {
        // Linear interpolieren zwischen Ausgangs- und Zielpunkt
        Position curPos(GAMECLIENT.Interpolate(startPos.x, destPos.x, event), GAMECLIENT.Interpolate(startPos.y, destPos.y, event));
        DrawPoint drawPos = curPos - drawOffset + worldSize;
        drawPos.x %= worldSize.x;
        drawPos.y %= worldSize.y;
        // Schatten auf linearer Linie zeichnen
        LOADER.GetMapImageN(3101)->DrawFull(drawPos, COLOR_SHADOW);

        Position distance = destPos - startPos;
        double whole = std::sqrt(double(distance.x * distance.x + distance.y * distance.y));
        unsigned s = GAMECLIENT.Interpolate(static_cast<unsigned>(whole), event);

        double dx = double(s) / whole - 0.5;

        // Y-Verschiebung ausrechnen, damit die Nullpunkte beim Start- und Endpunkt liegen
        double y_diff = 0.5 * 0.5;

        // Verschiebung ausrechnen von Y
        int diff = int((dx * dx - y_diff) * 200);

        // Stein auf Parabel zeichnen
        drawPos.y = (drawPos.y + diff) % worldSize.y;
        LOADER.GetMapPlayerImage(3100)->DrawFull(drawPos);
    }
}

void CatapultStone::HandleEvent(const unsigned /*id*/)
{
    if(explode)
    {
        // Explodiert --> mich zerstören
        gwg->RemoveCatapultStone(this);
        GetEvMgr().AddToKillList(this);
    } else
    {
        // Stein ist aufgeschlagen --> Explodierevent anmelden
        event = GetEvMgr().AddEvent(this, 10);
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
                if(milBld->GetNumTroops() == 0)
                    gwg->DestroyNO(milBld->GetPos());
            }
        } else
        {
            // Trifft nicht
            // ggf. Leiche hinlegen, falls da nix ist
            if(!gwg->GetSpecObj<noBase>(dest_map))
                gwg->SetNO(dest_map, new noEnvObject(dest_map, 502 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2)));
        }
    }
}
