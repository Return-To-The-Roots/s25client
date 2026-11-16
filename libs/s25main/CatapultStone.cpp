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
    event = getEvMgr().addEvent(this, fly_duration);
}

CatapultStone::CatapultStone(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), dest_building(sgd.popMapPoint()), dest_map(sgd.popMapPoint()),
      startPos(helpers::popPoint<Position>(sgd)), destPos(helpers::popPoint<Position>(sgd)), explode(sgd.PopBool()),
      event(sgd.popEvent())
{}

/// Serialisierungsfunktionen
void CatapultStone::Serialize(SerializedGameData& sgd) const
{
    helpers::pushPoint(sgd, dest_building);
    helpers::pushPoint(sgd, dest_map);
    helpers::pushPoint(sgd, startPos);
    helpers::pushPoint(sgd, destPos);
    sgd.PushBool(explode);
    sgd.pushEvent(event);
}

void CatapultStone::destroy() {}

void CatapultStone::draw(DrawPoint drawOffset)
{
    const DrawPoint worldSize = DrawPoint(world->getWidth() * TR_W, world->getHeight() * TR_H);

    if(explode)
    {
        // Stein explodierend am Ziel zeichnen
        DrawPoint drawPos = destPos - drawOffset + worldSize;
        drawPos.x %= worldSize.x;
        drawPos.y %= worldSize.y;
        LOADER.getMapTexture(3102 + GAMECLIENT.interpolate(4, event))->drawFull(drawPos);
    } else
    {
        // Linear interpolieren zwischen Ausgangs- und Zielpunkt
        Position curPos(GAMECLIENT.interpolate(startPos.x, destPos.x, event),
                        GAMECLIENT.interpolate(startPos.y, destPos.y, event));
        DrawPoint drawPos = curPos - drawOffset + worldSize;
        drawPos.x %= worldSize.x;
        drawPos.y %= worldSize.y;
        // Schatten auf linearer Linie zeichnen
        LOADER.getMapTexture(3101)->drawFull(drawPos, COLOR_SHADOW);

        Position distance = destPos - startPos;
        double whole = std::sqrt(double(distance.x * distance.x + distance.y * distance.y));
        unsigned s = GAMECLIENT.interpolate(static_cast<unsigned>(whole), event);

        double dx = double(s) / whole - 0.5;

        // Y-Verschiebung ausrechnen, damit die Nullpunkte beim Start- und Endpunkt liegen
        double y_diff = 0.5 * 0.5;

        // Verschiebung ausrechnen von Y
        auto diff = int((dx * dx - y_diff) * 200);

        // Stein auf Parabel zeichnen
        drawPos.y = (drawPos.y + diff) % worldSize.y;
        LOADER.getMapTexture(3100)->drawFull(drawPos);
    }
}

void CatapultStone::handleEvent(const unsigned /*id*/)
{
    if(explode)
    {
        // Explodiert --> mich zerstören
        world->removeCatapultStone(this);
        getEvMgr().addToKillList(this);
    } else
    {
        // Stein ist aufgeschlagen --> Explodierevent anmelden
        event = getEvMgr().addEvent(this, 10);
        explode = true;

        // Trifft der Stein?
        if(dest_building == dest_map)
        {
            // Steht an der Stelle noch ein Militärgebäude zum Bombardieren?
            auto* milBld = world->getSpecObj<nobMilitary>(dest_building);
            if(milBld)
            {
                milBld->hitOfCatapultStone();
                // If there are no troops left, destroy it
                if(milBld->getNumTroops() == 0)
                    world->destroyNO(milBld->getPos());
            }
        } else
        {
            // Trifft nicht
            // ggf. Leiche hinlegen, falls da nix ist
            if(!world->getSpecObj<noBase>(dest_map))
                world->setNO(dest_map, new noEnvObject(dest_map, 502 + RANDOM_RAND(2)));
        }
    }
}
