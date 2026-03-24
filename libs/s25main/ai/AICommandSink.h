// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "factories/GameCommandFactory.h"
#include "gameTypes/ChatDestination.h"
#include <memory>
#include <vector>

class GameMessage_Chat;
class GamePlayer;
class noBuilding;
class noFlag;
class noShip;
class nobHarborBuilding;
class nobMilitary;
class nobShipYard;

class AICommandSink : public GameCommandFactory
{
public:
    AICommandSink(std::vector<gc::GameCommandPtr>& gcs, const GamePlayer& player, unsigned char playerID);
    ~AICommandSink() override;

    using GameCommandFactory::CallSpecialist;
    using GameCommandFactory::CancelExpedition;
    using GameCommandFactory::DestroyBuilding;
    using GameCommandFactory::DestroyFlag;
    using GameCommandFactory::FoundColony;
    using GameCommandFactory::SetCoinsAllowed;
    using GameCommandFactory::SetShipYardMode;
    using GameCommandFactory::StartStopExpedition;
    using GameCommandFactory::TravelToNextSpot;

    bool SetCoinsAllowed(const nobMilitary* building, bool enabled);
    bool StartStopExpedition(const nobHarborBuilding* hb, bool start);
    bool FoundColony(const noShip* ship);
    bool TravelToNextSpot(ShipDirection direction, const noShip* ship);
    bool CancelExpedition(const noShip* ship);
    bool SetShipYardMode(const nobShipYard* shipyard, bool buildShips);
    bool DestroyBuilding(const noBuilding* building);
    bool DestroyFlag(const noFlag* flag);
    bool CallSpecialist(const noFlag* flag, Job job);

    void Chat(const std::string& message, ChatDestination destination = ChatDestination::All);
    std::vector<std::unique_ptr<GameMessage_Chat>> FetchChatMessages();

protected:
    bool AddGC(gc::GameCommandPtr gc) override;

private:
    const GamePlayer& player_;
    std::vector<gc::GameCommandPtr>& gcs_;
    std::vector<std::unique_ptr<GameMessage_Chat>> pendingChatMsgs_;
    const unsigned char playerID_;
};
