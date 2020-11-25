#pragma once
#include "GameObject.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Inventory.h"
#include "gameData/MaxPlayers.h"

#include <vector>

class SerializedGameData;
class GameEvent;

auto getPlayerMask = [](unsigned playerId) { return 1u << playerId; };

// Handler object to keep track of the economy mode progress and for the game end event
class EconomyModeHandler : public GameObject
{
public:
    static const unsigned int numGoodTypesToCollect = 7;

    struct econTeam
    {
        unsigned mask;
        unsigned teamAmounts[numGoodTypesToCollect];
        unsigned num_players_in_team;
        unsigned teamWins;

        econTeam(unsigned mask, unsigned num_players_in_team) : mask(mask), num_players_in_team(num_players_in_team)
        {
            for (unsigned int i = 0; i < numGoodTypesToCollect; i++) {
                teamAmounts[i] = 0;
            }
            teamWins = 0;
        }
    };

private:
    ///Frame in which the game is going to end
    unsigned end_frame;
    /// End game Event
    const GameEvent* event;
    /// Good types to collect
    GoodType types[numGoodTypesToCollect];

    //Data for economy mode progress tracking
    std::vector<econTeam> teams;
    unsigned maxTeamAmounts[numGoodTypesToCollect] = {0};
    unsigned int mostWins = 0;
    unsigned amounts[numGoodTypesToCollect][MAX_PLAYERS];
    unsigned last_updated;

    unsigned int SumGood(GoodType good, const Inventory& Inventory);

    void FindTeams();

public:

    EconomyModeHandler(unsigned end_frame);

    EconomyModeHandler(SerializedGameData& sgd, unsigned objId);

    /// Destroy
    void Destroy() override;

    void Serialize(SerializedGameData& sgd) const override;

    /// Event-Handler
    void HandleEvent(unsigned id) override;

    const std::vector<econTeam>& GetTeams()
    {
        FindTeams();
        return teams;
    }

    //Methods to update the ware trackers
    void UpdateAmounts();
    unsigned int GetAmount(unsigned int i, unsigned int player) { return amounts[i][player]; }
    unsigned int GetMaxTeamAmount(unsigned int i) { return maxTeamAmounts[i]; }

    unsigned GetEndFrame() { return end_frame; }

    // Check if the game has ended, so everything should be visible
    bool globalVisibility();

    //Return the good types to collect
    GoodType* GetTypes() { return types; }

    GO_Type GetGOT() const override { return GOT_ECONOMYMODEHANDLER; }

    bool isOver() const;
    
};
