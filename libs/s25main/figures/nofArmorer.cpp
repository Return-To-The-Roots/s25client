// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofArmorer.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

nofArmorer::nofArmorer(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Armorer, pos, player, workplace), sword_shield(false)
{}

void nofArmorer::serialize(SerializedGameData& sgd) const
{
    nofWorkman::serialize(sgd);

    sgd.pushBool(sword_shield);
}

nofArmorer::nofArmorer(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id), sword_shield(sgd.popBool())
{}

void nofArmorer::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{-10, 15}, {-11, 9}, {-14, 16}, {-19, 1}, {-11, 9}}};

    constexpr unsigned maxId = 280;
    const unsigned nowId = GAMECLIENT.interpolate(maxId, current_ev);
    if(nowId < 200)
    {
        const Nation wpNation = workplace->getNation();
        const unsigned plColor = world->getPlayer(player).color;

        LOADER.getPlayerImage("rom_bobs", 16 + (nowId % 8))->drawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);

        if((nowId % 8) == 5)
        {
            world->getSoundMgr().playNOSound(52, *this, nowId / 8);
            was_sounding = true;
        }
    }
}

unsigned short nofArmorer::getCarryID() const
{
    if(sword_shield)
        return 56;
    else
    {
        // Je nach Nation einen bestimmtem Schild fertigen
        switch(world->getPlayer(player).nation)
        {
            case Nation::Africans: return 60; //-V525
            case Nation::Japanese: return 58;
            case Nation::Romans: return 57;
            case Nation::Vikings: return 59;
            case Nation::Babylonians: return 58; // babylonians use japanese shield carry-animation
        }
        RTTR_Assert(false);
        return 0;
    }
}

void nofArmorer::handleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::Waiting1:
        {
            if(!world->getGGS().isEnabled(AddonId::HALF_COST_MIL_EQUIP) || !sword_shield)
            {
                // LOG.write(("armorer handlewait1 - consume wares %i \n",player);
                nofWorkman::handleStateWaiting1();
            } else
            {
                // Nach 1. Warten wird gearbeitet
                current_ev = getEvMgr().addEvent(this, JOB_CONSTS[job_].work_length, 1);
                state = State::Work;
                workplace->is_working = true;
                // LOG.write(("armorer handlewait1 - no consume wares %i \n",player);
            }
        }
        break;
        case State::Work:
        {
            handleStateWork();
        }
        break;
        case State::Waiting2:
        {
            handleStateWaiting2();
        }
        break;
        default: break;
    }
}

bool nofArmorer::areWaresAvailable() const
{
    return workplace->waresAvailable() || (world->getGGS().isEnabled(AddonId::HALF_COST_MIL_EQUIP) && sword_shield);
}

helpers::OptionalEnum<GoodType> nofArmorer::produceWare()
{
    sword_shield = !sword_shield;

    if(sword_shield)
        return GoodType::Sword;
    else
        return SHIELD_TYPES[world->getPlayer(player).nation];
}
