// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CatapultStone.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noEnvObject.h"
#include "gameData/MapConsts.h"

#include <cmath>

CatapultStone::CatapultStone(const MapPoint dest_building, const MapPoint dest_map, const DrawPoint start,
                             const DrawPoint dest, const unsigned fly_duration)
    : dest_building(dest_building), dest_map(dest_map), startPos(start), destPos(dest), explode(false)
{
    event = GetEvMgr().AddEvent(this, fly_duration);
}

CatapultStone::CatapultStone(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), dest_building(sgd.PopMapPoint()), dest_map(sgd.PopMapPoint()),
      startPos(helpers::popPoint<Position>(sgd)), destPos(helpers::popPoint<Position>(sgd)), explode(sgd.PopBool()),
      event(sgd.PopEvent())
{}

/// Serialisierungsfunktionen
void CatapultStone::Serialize(SerializedGameData& sgd) const
{
    helpers::pushPoint(sgd, dest_building);
    helpers::pushPoint(sgd, dest_map);
    helpers::pushPoint(sgd, startPos);
    helpers::pushPoint(sgd, destPos);
    sgd.PushBool(explode);
    sgd.PushEvent(event);
}

void CatapultStone::Destroy() {}

void CatapultStone::Draw(DrawPoint drawOffset)
{
    const DrawPoint worldSize = DrawPoint(world->GetWidth() * TR_W, world->GetHeight() * TR_H);

    if(explode)
    {
        // Stein explodierend am Ziel zeichnen
        DrawPoint drawPos = destPos - drawOffset + worldSize;
        drawPos.x %= worldSize.x;
        drawPos.y %= worldSize.y;
        LOADER.GetMapTexture(3102 + GAMECLIENT.Interpolate(4, event))->DrawFull(drawPos);
    } else
    {
        // Linear interpolieren zwischen Ausgangs- und Zielpunkt
        Position curPos(GAMECLIENT.Interpolate(startPos.x, destPos.x, event),
                        GAMECLIENT.Interpolate(startPos.y, destPos.y, event));
        DrawPoint drawPos = curPos - drawOffset + worldSize;
        drawPos.x %= worldSize.x;
        drawPos.y %= worldSize.y;
        // Schatten auf linearer Linie zeichnen
        LOADER.GetMapTexture(3101)->DrawFull(drawPos, COLOR_SHADOW);

        Position distance = destPos - startPos;
        double whole = std::sqrt(double(distance.x * distance.x + distance.y * distance.y));
        unsigned s = GAMECLIENT.Interpolate(static_cast<unsigned>(whole), event);

        double dx = double(s) / whole - 0.5;

        // Y-Verschiebung ausrechnen, damit die Nullpunkte beim Start- und Endpunkt liegen
        double y_diff = 0.5 * 0.5;

        // Verschiebung ausrechnen von Y
        auto diff = int((dx * dx - y_diff) * 200);

        // Stein auf Parabel zeichnen
        drawPos.y = (drawPos.y + diff) % worldSize.y;
        LOADER.GetMapTexture(3100)->DrawFull(drawPos);
    }
}

void CatapultStone::HandleEvent(const unsigned /*id*/)
{
    if(explode)
    {
        // Explodiert --> mich zerstören
        world->RemoveCatapultStone(this);
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
            auto* milBld = world->GetSpecObj<nobMilitary>(dest_building);
            if(milBld)
            {
                milBld->HitOfCatapultStone();
                // If there are no troops left, destroy it
                if(milBld->GetNumTroops() == 0)
                    world->DestroyNO(milBld->GetPos());
            }
        } else
        {
            // Trifft nicht
            // ggf. Leiche hinlegen, falls da nix ist
            if(!world->GetSpecObj<noBase>(dest_map))
                world->SetNO(dest_map, new noEnvObject(dest_map, 502 + RANDOM_RAND(2)));
        }
    }
}
