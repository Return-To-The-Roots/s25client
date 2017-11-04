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

#include "defines.h" // IWYU pragma: keep
#include "nofMetalworker.h"

#include "EventManager.h"
#include "GameClient.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "buildings/nobUsual.h"
#include "notifications/NotificationManager.h"
#include "notifications/ToolNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "postSystem/PostMsg.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "gameData/ToolConsts.h"
#include "libutil/Log.h"
#include <boost/lambda/bind.hpp>
#include <boost/lambda/control_structures.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/lambda.hpp>

nofMetalworker::nofMetalworker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_METALWORKER, pos, player, workplace), nextProducedTool(GD_NOTHING)
{
    namespace bl = boost::lambda;
    using bl::_1;
    toolOrderSub = gwg->GetNotifications().subscribe<ToolNote>(
      bl::if_((bl::bind(&ToolNote::type, _1) == ToolNote::OrderPlaced || bl::bind(&ToolNote::type, _1) == ToolNote::SettingsChanged)
              && bl::bind(&ToolNote::player, _1) == boost::ref(player))[bl::bind(&nofMetalworker::CheckForOrders, this)]);
}

nofMetalworker::nofMetalworker(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id), nextProducedTool(GoodType(sgd.PopUnsignedChar()))
{
    if(state == STATE_ENTERBUILDING && current_ev == NULL && ware == GD_NOTHING && nextProducedTool == GD_NOTHING)
    {
        LOG.write("Found invalid metalworker. Assuming corrupted savegame -> Trying to fix this. If you encounter this with a new game, "
                  "report this!");
        RTTR_Assert(false);
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        current_ev = GetEvMgr().AddEvent(this, 1000, 2);
    }
    namespace bl = boost::lambda;
    using bl::_1;
    toolOrderSub = gwg->GetNotifications().subscribe<ToolNote>(
      bl::if_((bl::bind(&ToolNote::type, _1) == ToolNote::OrderPlaced || bl::bind(&ToolNote::type, _1) == ToolNote::SettingsChanged)
              && bl::bind(&ToolNote::player, _1) == boost::ref(player))[bl::bind(&nofMetalworker::CheckForOrders, this)]);
}

void nofMetalworker::Serialize(SerializedGameData& sgd) const
{
    nofWorkman::Serialize(sgd);
    sgd.PushUnsignedChar(nextProducedTool);
}

void nofMetalworker::DrawWorking(DrawPoint drawPt)
{
    const DrawPointInit offsets[NAT_COUNT] = {{-11, -13}, {31, 5}, {32, 6}, {30, 10}, {28, 5}};

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
const unsigned short CARRYTOOLS_IDS[TOOL_COUNT] = {78, 79, 80, 91, 81, 82, 83, 84, 85, 87, 88, 90};

unsigned short nofMetalworker::GetCarryID() const
{
    for(unsigned i = 0; i < TOOL_COUNT; i++)
        if(TOOLS[i] == ware)
            return CARRYTOOLS_IDS[i];
    return 0;
}

bool nofMetalworker::HasToolOrder() const
{
    const GamePlayer& owner = gwg->GetPlayer(player);
    for(unsigned i = 0; i < TOOL_COUNT; ++i)
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
    for(unsigned i = 0; i < TOOL_COUNT; ++i)
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
    // qx:tools
    int maxPrio = -1;
    int tool = -1;

    GamePlayer& owner = gwg->GetPlayer(player);
    for(unsigned i = 0; i < TOOL_COUNT; ++i)
    {
        if(owner.GetToolsOrdered(i) > 0u && static_cast<int>(owner.GetToolPriority(i)) > maxPrio)
        {
            maxPrio = owner.GetToolPriority(i);
            tool = i;
        }
    }

    if(tool != -1)
    {
        owner.ToolOrderProcessed(tool);

        if(HasToolOrder() == 0)
            SendPostMessage(player,
                            new PostMsg(GetEvMgr().GetCurrentGF(), _("Completed the ordered amount of tools."), PostCategory::Economy));

        return TOOLS[tool];
    }
    return GD_NOTHING;
}

GoodType nofMetalworker::GetRandomTool()
{
    const GamePlayer& owner = gwg->GetPlayer(player);

    // Je nach Werkzeugeinstellungen zufällig ein Werkzeug produzieren, je größer der Balken,
    // desto höher jeweils die Wahrscheinlichkeit
    unsigned short all_size = 0;

    for(unsigned i = 0; i < TOOL_COUNT; ++i)
        all_size += owner.GetToolPriority(i);

    // if they're all zero
    if(all_size == 0)
    {
        // do nothing if addon is enabled, otherwise produce random ware (orig S2 behaviour)
        if(gwg->GetGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 1)
            return GD_NOTHING;
        else
            return TOOLS[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), TOOLS.size())];
    }

    // Ansonsten Array mit den Werkzeugtypen erstellen und davon dann eins zufällig zurückliefern, je höher Wahr-
    // scheinlichkeit (Balken), desto öfter im Array enthalten
    std::vector<unsigned char> random_array(all_size);
    unsigned curIdx = 0;

    for(unsigned i = 0; i < TOOL_COUNT; ++i)
    {
        for(unsigned g = 0; g < owner.GetToolPriority(i); ++g)
            random_array[curIdx++] = i;
    }

    GoodType tool = TOOLS[random_array[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), all_size)]];

    return tool;
}

GoodType nofMetalworker::ProduceWare()
{
    return nextProducedTool;
}
