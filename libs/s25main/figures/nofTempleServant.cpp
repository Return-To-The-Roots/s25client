// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofTempleServant.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "WineLoader.h"
#include "buildings/nobTemple.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"

using namespace wineaddon;

nofTempleServant::nofTempleServant(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::TempleServant, pos, player, workplace), currentProduction(GoodType::IronOre)
{}

nofTempleServant::nofTempleServant(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id), currentProduction(sgd.pop<GoodType>())
{}

void nofTempleServant::serialize(SerializedGameData& sgd) const
{
    nofWorkman::serialize(sgd);
    sgd.pushEnum<uint8_t>(currentProduction);
}

void nofTempleServant::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{26, -23}, {17, -40}, {4, 0}, {22, -25}, {20, -45}}};

    const unsigned now_id = GAMECLIENT.interpolate(82, current_ev);

    if(now_id < 11) // servant pours wine into crucible(play once at start)
    {
        LOADER.getPlayerImage("wine_bobs", bobIndex[BobTypes::TEMPLE_WORK_WINDOW_START_ANIMATION] + now_id)
          ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE,
                     world->getPlayer(workplace->getPlayer()).color);
    } else if(now_id < 77) // wine and food is burned (loop for duration)
    {
        LOADER.getPlayerImage("wine_bobs", bobIndex[BobTypes::TEMPLE_WORK_WINDOW_MAIN_ANIMATION] + (now_id + 1) % 6)
          ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE,
                     world->getPlayer(workplace->getPlayer()).color);

        // Play sound burning
        if((now_id + 1) % 6 == 3)
        {
            world->getSoundMgr().playNOSound(93, *this, now_id);
            was_sounding = true;
        }
    } else // last loop of animation (play once at end)
    {
        LOADER.getPlayerImage("wine_bobs", bobIndex[BobTypes::TEMPLE_WORK_WINDOW_END_ANIMATION] + now_id - 77)
          ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE,
                     world->getPlayer(workplace->getPlayer()).color);
    }

    last_id = now_id;
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofTempleServant::getCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

void nofTempleServant::drawWalkingWithWare(DrawPoint drawPt)
{
    std::map<GoodType, BobTypes> goodTypeToTexture{{GoodType::Gold, BobTypes::TEMPLESERVANT_CARRYING_GOLD_IN_OUT},
                                                   {GoodType::IronOre, BobTypes::TEMPLESERVANT_CARRYING_IRON_IN_OUT},
                                                   {GoodType::Coal, BobTypes::TEMPLESERVANT_CARRYING_COAL_IN_OUT},
                                                   {GoodType::Stones, BobTypes::TEMPLESERVANT_CARRYING_STONE_IN_OUT}};

    drawWalking(drawPt, "wine_bobs", bobIndex[goodTypeToTexture[currentProduction]]);
}

helpers::OptionalEnum<GoodType> nofTempleServant::produceWare()
{
    const auto productionMode = static_cast<nobTemple*>(workplace)->getProductionMode();
    switch(productionMode)
    {
        case ProductionMode::IronOre: currentProduction = GoodType::IronOre; break;
        case ProductionMode::Coal: currentProduction = GoodType::Coal; break;
        case ProductionMode::Stone: currentProduction = GoodType::Stones; break;
        default:
            constexpr std::array<GoodType, 4> resultWare{GoodType::Gold, GoodType::IronOre, GoodType::Coal,
                                                         GoodType::Stones};
            currentProduction = resultWare[RANDOM_RAND(4)];
            break;
    }
    return currentProduction;
}
