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
#include "world/GameWorldGame.h"
#include "gameData/ToolConsts.h"
#include "s25util/Log.h"

nofMetalworker::nofMetalworker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_METALWORKER, pos, player, workplace), nextProducedTool(GD_NOTHING)
{
    toolOrderSub = gwg->GetNotifications().subscribe<ToolNote>([this](const ToolNote& note) {
        if((note.type == ToolNote::OrderPlaced || note.type == ToolNote::SettingsChanged) && note.player == this->player)
            CheckForOrders();
    });
}

nofMetalworker::nofMetalworker(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id), nextProducedTool(GoodType(sgd.PopUnsignedChar()))
{
    if(state == STATE_ENTERBUILDING && current_ev == nullptr && ware == GD_NOTHING && nextProducedTool == GD_NOTHING)
    {
        LOG.write("Found invalid metalworker. Assuming corrupted savegame -> Trying to fix this. If you encounter this with a new game, "
                  "report this!");
        RTTR_Assert(false);
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        current_ev = GetEvMgr().AddEvent(this, 1000, 2);
    }
    toolOrderSub = gwg->GetNotifications().subscribe<ToolNote>([this](const ToolNote& note) {
        if((note.type == ToolNote::OrderPlaced || note.type == ToolNote::SettingsChanged) && note.player == this->player)
            CheckForOrders();
    });
}

void nofMetalworker::Serialize(SerializedGameData& sgd) const
{
    nofWorkman::Serialize(sgd);
    sgd.PushUnsignedChar(nextProducedTool);
}

void nofMetalworker::DrawWorking(DrawPoint drawPt)
{
    const std::array<DrawPoint, NUM_NATS> offsets = {{{-11, -13}, {31, 5}, {32, 6}, {30, 10}, {28, 5}}};

    const unsigned now_id = GAMECLIENT.Interpolate(230, current_ev);

    LOADER.GetPlayerImage("rom_bobs", 190 + (now_id % 23))
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, gwg->GetPlayer(workplace->GetPlayer()).color);

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

// Zuordnungnen Richtige IDs - Trage-IDs in der JOBS.BOB
const std::array<uint8_t, NUM_TOOLS> CARRYTOOLS_IDS = {78, 79, 80, 91, 81, 82, 83, 84, 85, 87, 88, 86};

unsigned short nofMetalworker::GetCarryID() const
{
    const int toolIdx = helpers::indexOf(TOOLS, ware);
    return (toolIdx >= 0) ? CARRYTOOLS_IDS[toolIdx] : 0;
}

bool nofMetalworker::HasToolOrder() const
{
    const GamePlayer& owner = gwg->GetPlayer(player);
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
    if(gwg->GetGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 0)
        return true;
    // Any tool order?
    if(HasToolOrder())
        return true;
    // Any non-zero priority?
    const GamePlayer& owner = gwg->GetPlayer(player);
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
    if(nextProducedTool == GD_NOTHING)
        nextProducedTool = GetRandomTool();

    return (nextProducedTool != GD_NOTHING) && nofWorkman::StartWorking();
}

void nofMetalworker::CheckForOrders()
{
    // If we are waiting and an order or setting was changed -> See if we can work
    if(state == STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED)
        TryToWork();
}

GoodType nofMetalworker::GetOrderedTool()
{
    GamePlayer& owner = gwg->GetPlayer(player);
    std::vector<uint8_t> random_array;
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        if(owner.GetToolsOrdered(i) == 0)
            continue;
        unsigned toolPriority = std::max(owner.GetToolPriority(i), 1u);
        random_array.insert(random_array.end(), toolPriority, i);
    }
    if(random_array.empty())
        return GD_NOTHING;

    unsigned toolIdx = random_array[RANDOM_RAND(GetObjId(), random_array.size())];

    owner.ToolOrderProcessed(toolIdx);

    if(!HasToolOrder())
        SendPostMessage(player, new PostMsg(GetEvMgr().GetCurrentGF(), _("Completed the ordered amount of tools."), PostCategory::Economy));

    return TOOLS[toolIdx];
}

GoodType nofMetalworker::GetRandomTool()
{
    GamePlayer& owner = gwg->GetPlayer(player);

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
        if(gwg->GetGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 1)
            return GD_NOTHING;
        else
            return TOOLS[RANDOM_RAND(GetObjId(), TOOLS.size())];
    }

    return TOOLS[random_array[RANDOM_RAND(GetObjId(), random_array.size())]];
}

GoodType nofMetalworker::ProduceWare()
{
    return nextProducedTool;
}
