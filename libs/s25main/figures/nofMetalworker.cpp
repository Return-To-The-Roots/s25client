// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
    toolOrderSub = world->GetNotifications().subscribe<ToolNote>([this](const ToolNote& note) {
        if((note.type == ToolNote::OrderPlaced || note.type == ToolNote::SettingsChanged)
           && note.player == this->player)
            CheckForOrders();
    });
}

nofMetalworker::nofMetalworker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
    if(sgd.GetGameDataVersion() < 5)
    {
        const auto iWare = sgd.PopUnsignedChar();
        if(iWare == rttr::enum_cast(GoodType::Nothing))
            nextProducedTool = boost::none;
        else
            nextProducedTool = GoodType(iWare);
    } else
        nextProducedTool = sgd.PopOptionalEnum<GoodType>();
    if(state == State::EnterBuilding && current_ev == nullptr && !ware && !nextProducedTool)
    {
        LOG.write("Found invalid metalworker. Assuming corrupted savegame -> Trying to fix this. If you encounter this "
                  "with a new game, "
                  "report this!");
        RTTR_Assert(false);
        state = State::WaitingForWaresOrProductionStopped;
        current_ev = GetEvMgr().AddEvent(this, 1000, 2);
    }
    toolOrderSub = world->GetNotifications().subscribe<ToolNote>([this](const ToolNote& note) {
        if((note.type == ToolNote::OrderPlaced || note.type == ToolNote::SettingsChanged)
           && note.player == this->player)
            CheckForOrders();
    });
}

void nofMetalworker::Serialize(SerializedGameData& sgd) const
{
    nofWorkman::Serialize(sgd);
    sgd.PushOptionalEnum<uint8_t>(nextProducedTool);
}

void nofMetalworker::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{-11, -13}, {31, 5}, {32, 6}, {30, 10}, {28, 5}}};

    const unsigned now_id = GAMECLIENT.Interpolate(230, current_ev);

    LOADER.GetPlayerImage("rom_bobs", 190 + (now_id % 23))
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);

    // Hämmer-Sound
    if(now_id % 23 == 3 || now_id % 23 == 7)
    {
        SOUNDMANAGER.PlayNOSound(72, this, now_id, 100);
        was_sounding = true;
    }
    // Säge-Sound 1
    else if(now_id % 23 == 9)
    {
        SOUNDMANAGER.PlayNOSound(54, this, now_id);
        was_sounding = true;
    } else if(now_id % 23 == 17)
    {
        SOUNDMANAGER.PlayNOSound(55, this, now_id);
        was_sounding = true;
    }

    last_id = now_id;
}

// Mapping of indices in TOOLS to IDs in JOBS.BOB
constexpr std::array<uint8_t, NUM_TOOLS> CARRYTOOLS_IDS = {78, 79, 80, 91, 81, 82, 83, 84, 85, 87, 88, 86};

unsigned short nofMetalworker::GetCarryID() const
{
    const int toolIdx = helpers::indexOf(TOOLS, ware);
    return (toolIdx >= 0) ? CARRYTOOLS_IDS[toolIdx] : 0;
}

bool nofMetalworker::HasToolOrder() const
{
    const GamePlayer& owner = world->GetPlayer(player);
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        if(owner.GetToolsOrdered(i) > 0u)
            return true;
    }
    return false;
}

bool nofMetalworker::AreWaresAvailable() const
{
    if(!nofWorkman::AreWaresAvailable())
        return false;
    // If produce nothing on zero is disabled we will always produce something ->OK
    if(world->GetGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 0)
        return true;
    // Any tool order?
    if(HasToolOrder())
        return true;
    // Any non-zero priority?
    const GamePlayer& owner = world->GetPlayer(player);
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        if(owner.GetToolPriority(i) > 0u)
            return true;
    }
    return false;
}

bool nofMetalworker::StartWorking()
{
    nextProducedTool = GetOrderedTool();
    if(!nextProducedTool)
        nextProducedTool = GetRandomTool();

    return nextProducedTool && nofWorkman::StartWorking();
}

void nofMetalworker::CheckForOrders()
{
    // If we are waiting and an order or setting was changed -> See if we can work
    if(state == State::WaitingForWaresOrProductionStopped)
        TryToWork();
}

helpers::OptionalEnum<GoodType> nofMetalworker::GetOrderedTool()
{
    GamePlayer& owner = world->GetPlayer(player);
    std::vector<uint8_t> random_array;
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        if(owner.GetToolsOrdered(i) == 0)
            continue;
        unsigned toolPriority = std::max(owner.GetToolPriority(i), 1u);
        random_array.insert(random_array.end(), toolPriority, i);
    }
    if(random_array.empty())
        return boost::none;

    unsigned toolIdx = RANDOM_ELEMENT(random_array);

    owner.ToolOrderProcessed(toolIdx);

    if(!HasToolOrder())
        SendPostMessage(player,
                        std::make_unique<PostMsg>(GetEvMgr().GetCurrentGF(),
                                                  _("Completed the ordered amount of tools."), PostCategory::Economy));

    return TOOLS[toolIdx];
}

helpers::OptionalEnum<GoodType> nofMetalworker::GetRandomTool()
{
    GamePlayer& owner = world->GetPlayer(player);

    // Fill array where the # of occurrences of a tool is its priority
    // Drawing a random entry will make higher priority items more likely
    std::vector<uint8_t> random_array;
    random_array.reserve(TOOLS.size());
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        random_array.insert(random_array.end(), owner.GetToolPriority(i), i);
    }

    // if they're all zero
    if(random_array.empty())
    {
        // do nothing if addon is enabled, otherwise produce random ware (orig S2 behavior)
        if(world->GetGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 1)
            return boost::none;
        else
            return RANDOM_ELEMENT(TOOLS);
    }

    return TOOLS[RANDOM_ELEMENT(random_array)];
}

helpers::OptionalEnum<GoodType> nofMetalworker::ProduceWare()
{
    return nextProducedTool;
}
