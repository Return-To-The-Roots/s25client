// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICommandSink.h"

#include "GamePlayer.h"
#include "buildings/noBuilding.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "network/GameMessage_Chat.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"

AICommandSink::AICommandSink(std::vector<gc::GameCommandPtr>& gcs, const GamePlayer& player, unsigned char playerID)
    : player_(player), gcs_(gcs), playerID_(playerID)
{}

AICommandSink::~AICommandSink() = default;

bool AICommandSink::SetCoinsAllowed(const nobMilitary* building, const bool enabled)
{
    return SetCoinsAllowed(building->GetPos(), enabled);
}

bool AICommandSink::StartStopExpedition(const nobHarborBuilding* hb, bool start)
{
    return StartStopExpedition(hb->GetPos(), start);
}

bool AICommandSink::FoundColony(const noShip* ship)
{
    return FoundColony(player_.GetShipID(ship));
}

bool AICommandSink::TravelToNextSpot(ShipDirection direction, const noShip* ship)
{
    return TravelToNextSpot(direction, player_.GetShipID(ship));
}

bool AICommandSink::CancelExpedition(const noShip* ship)
{
    return CancelExpedition(player_.GetShipID(ship));
}

bool AICommandSink::SetShipYardMode(const nobShipYard* shipyard, bool buildShips)
{
    return SetShipYardMode(shipyard->GetPos(), buildShips);
}

bool AICommandSink::DestroyBuilding(const noBuilding* building)
{
    return DestroyBuilding(building->GetPos());
}

bool AICommandSink::DestroyFlag(const noFlag* flag)
{
    return DestroyFlag(flag->GetPos());
}

bool AICommandSink::CallSpecialist(const noFlag* flag, Job job)
{
    return CallSpecialist(flag->GetPos(), job);
}

void AICommandSink::Chat(const std::string& message, ChatDestination destination)
{
    pendingChatMsgs_.push_back(std::make_unique<GameMessage_Chat>(playerID_, destination, message));
}

std::vector<std::unique_ptr<GameMessage_Chat>> AICommandSink::FetchChatMessages()
{
    std::vector<std::unique_ptr<GameMessage_Chat>> tmp;
    std::swap(tmp, pendingChatMsgs_);
    return tmp;
}

bool AICommandSink::AddGC(gc::GameCommandPtr gc)
{
    gcs_.push_back(gc);
    return true;
}
