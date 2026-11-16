// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofStonemason.h"

#include "GameInterface.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noGranite.h"

nofStonemason::nofStonemason(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Stonemason, pos, player, workplace)
{}

nofStonemason::nofStonemason(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Malt den Arbeiter beim Arbeiten
void nofStonemason::drawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    // Stein hauen
    LOADER.getPlayerImage("rom_bobs", 40 + (now_id = GAMECLIENT.interpolate(64, current_ev)) % 8)
      ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);

    if(now_id % 8 == 5)
    {
        world->getSoundMgr().playNOSound(56, *this, now_id);
        was_sounding = true;
    }
}

unsigned short nofStonemason::getCarryID() const
{
    return 63;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofStonemason::workStarted() {}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofStonemason::workFinished()
{
    // Stein abhauen (wenn er nur noch ganz klein ist, dann wird er von der Landkarte getilgt)
    if(world->getSpecObj<noGranite>(pos)->isSmall())
    {
        // Granitklötzchen löschen
        world->destroyNO(pos);

        // Minimap Bescheid geben (Granitglötzchen muss weg)
        if(world->getGameInterface())
            world->getGameInterface()->GI_UpdateMinimap(pos);

        // Drumherum BQ neu berechnen, da diese sich ja jetzt hätten ändern können
        world->recalcBQAroundPoint(pos);
    } else
        // ansonsten wird er um 1 kleiner
        world->getSpecObj<noGranite>(pos)->hew();

    // Stein in die Hand nehmen
    ware = GoodType::Stones;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofStonemason::getPointQuality(const MapPoint pt, bool /* isBeforeWork */) const
{
    // An dieser Position muss es nur Stein geben
    return ((world->getNO(pt)->getType() == NodalObjectType::Granite) ? PointQuality::Class1 :
                                                                        PointQuality::NotPossible);
}
