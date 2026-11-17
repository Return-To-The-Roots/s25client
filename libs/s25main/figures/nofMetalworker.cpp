// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofMetalworker.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "buildings/nobUsual.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "notifications/NotificationManager.h"
#include "notifications/ToolNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "postSystem/PostMsg.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/ToolConsts.h"
#include "s25util/Log.h"

nofMetalworker::nofMetalworker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Metalworker, pos, player, workplace)
{
    toolOrderSub = world->getNotifications().subscribe<ToolNote>([this](const ToolNote& note) {
        if((note.type == ToolNote::OrderPlaced || note.type == ToolNote::SettingsChanged)
           && note.player == this->player)
            checkForOrders();
    });
}

nofMetalworker::nofMetalworker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
    if(sgd.getGameDataVersion() < 5)
    {
        const auto iWare = sgd.popUnsignedChar();
        // GoodType::Nothing is moved because of adding new wares due addons
        // The old GoodType::Nothing is now GoodType::Grapes
        if(iWare == rttr::enumCast(GoodType::Grapes))
            nextProducedTool = std::nullopt;
        else
            nextProducedTool = GoodType(iWare);
    } else
        nextProducedTool = sgd.popOptionalEnum<GoodType>();
    if(state == State::EnterBuilding && current_ev == nullptr && !ware && !nextProducedTool)
    {
        LOG.write("Found invalid metalworker. Assuming corrupted savegame -> Trying to fix this. If you encounter this "
                  "with a new game, "
                  "report this!");
        RTTR_Assert(false);
        state = State::WaitingForWaresOrProductionStopped;
        current_ev = getEvMgr().addEvent(this, 1000, 2);
    }
    toolOrderSub = world->getNotifications().subscribe<ToolNote>([this](const ToolNote& note) {
        if((note.type == ToolNote::OrderPlaced || note.type == ToolNote::SettingsChanged)
           && note.player == this->player)
            checkForOrders();
    });
}

void nofMetalworker::serialize(SerializedGameData& sgd) const
{
    nofWorkman::serialize(sgd);
    sgd.pushOptionalEnum<uint8_t>(nextProducedTool);
}

void nofMetalworker::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{-11, -13}, {31, 5}, {32, 6}, {30, 10}, {28, 5}}};

    const unsigned now_id = GAMECLIENT.interpolate(230, current_ev);

    LOADER.getPlayerImage("rom_bobs", 190 + (now_id % 23))
      ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);

    // Hämmer-Sound
    if(now_id % 23 == 3 || now_id % 23 == 7)
    {
        world->getSoundMgr().playNOSound(72, *this, now_id, 100);
        was_sounding = true;
    }
    // Säge-Sound 1
    else if(now_id % 23 == 9)
    {
        world->getSoundMgr().playNOSound(54, *this, now_id);
        was_sounding = true;
    } else if(now_id % 23 == 17)
    {
        world->getSoundMgr().playNOSound(55, *this, now_id);
        was_sounding = true;
    }

    last_id = now_id;
}

// Mapping of indices in TOOLS to IDs in JOBS.BOB
constexpr helpers::EnumArray<uint8_t, Tool> CARRYTOOLS_IDS = {78, 79, 80, 91, 81, 82, 83, 84, 85, 87, 88, 86};

unsigned short nofMetalworker::getCarryID() const
{
    const auto toolIdx = helpers::indexOf(TOOL_TO_GOOD, ware);
    return (toolIdx >= 0) ? CARRYTOOLS_IDS[static_cast<Tool>(toolIdx)] : 0;
}

bool nofMetalworker::hasToolOrder() const
{
    const GamePlayer& owner = world->getPlayer(player);
    for(const auto tool : helpers::enumRange<Tool>())
    {
        if(owner.getToolsOrdered(tool) > 0u)
            return true;
    }
    return false;
}

bool nofMetalworker::areWaresAvailable() const
{
    if(!nofWorkman::areWaresAvailable())
        return false;
    // If produce nothing on zero is disabled we will always produce something ->OK
    if(world->getGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 0)
        return true;
    // Any tool order?
    if(hasToolOrder())
        return true;
    // Any non-zero priority?
    const GamePlayer& owner = world->getPlayer(player);
    for(const auto tool : helpers::enumRange<Tool>())
    {
        if(owner.getToolPriority(tool) > 0u)
            return true;
    }
    return false;
}

bool nofMetalworker::startWorking()
{
    nextProducedTool = getOrderedTool();
    if(!nextProducedTool)
        nextProducedTool = getRandomTool();

    return nextProducedTool && nofWorkman::startWorking();
}

void nofMetalworker::checkForOrders()
{
    // If we are waiting and an order or setting was changed -> See if we can work
    if(state == State::WaitingForWaresOrProductionStopped)
        tryToWork();
}

helpers::OptionalEnum<GoodType> nofMetalworker::getOrderedTool()
{
    GamePlayer& owner = world->getPlayer(player);
    std::vector<Tool> random_array;
    for(const auto tool : helpers::enumRange<Tool>())
    {
        if(owner.getToolsOrdered(tool) == 0)
            continue;
        unsigned toolPriority = std::max(owner.getToolPriority(tool), 1u);
        random_array.insert(random_array.end(), toolPriority, tool);
    }
    if(random_array.empty())
        return std::nullopt;

    const Tool tool = RANDOM_ELEMENT(random_array);

    owner.toolOrderProcessed(tool);

    if(!hasToolOrder())
    {
        sendPostMessage(player,
                        std::make_unique<PostMsg>(getEvMgr().getCurrentGF(),
                                                  _("Completed the ordered amount of tools."), PostCategory::Economy));
    }

    return TOOL_TO_GOOD[tool];
}

helpers::OptionalEnum<GoodType> nofMetalworker::getRandomTool()
{
    GamePlayer& owner = world->getPlayer(player);

    // Fill array where the # of occurrences of a tool is its priority
    // Drawing a random entry will make higher priority items more likely
    std::vector<Tool> random_array;
    random_array.reserve(helpers::NumEnumValues_v<Tool>);
    for(const auto tool : helpers::enumRange<Tool>())
    {
        random_array.insert(random_array.end(), owner.getToolPriority(tool), tool);
    }

    // if they're all zero
    if(random_array.empty())
    {
        // do nothing if addon is enabled, otherwise produce random ware (orig S2 behavior)
        if(world->getGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 1)
            return std::nullopt;
        else
            return RANDOM_ELEMENT(TOOL_TO_GOOD);
    }

    return TOOL_TO_GOOD[RANDOM_ELEMENT(random_array)];
}

helpers::OptionalEnum<GoodType> nofMetalworker::produceWare()
{
    return nextProducedTool;
}
