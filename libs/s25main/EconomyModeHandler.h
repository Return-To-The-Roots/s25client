#pragma once
#include "GameObject.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/Inventory.h"
#include "gameTypes/JobTypes.h"
#include "gameData/MaxPlayers.h"

#include <bitset>
#include <vector>

class SerializedGameData;
class GameEvent;

// Handler object to keep track of the economy mode progress and for the game end event
class EconomyModeHandler : public GameObject
{
public:
    // Object to hold data on the teams competing in the economy mode
    struct EconTeam
    {
        // The players that are in the team
        std::bitset<MAX_PLAYERS> playersInTeam;

        // The amounts the team collected for each good
        std::vector<unsigned> amountsTheTeamCollected;

        // The number of good types the team is the leader in
        unsigned goodTypeWins;

        EconTeam(std::bitset<MAX_PLAYERS> playersInTeam, unsigned numGoodTypesToCollect) noexcept
            : playersInTeam(playersInTeam), amountsTheTeamCollected(numGoodTypesToCollect, 0), goodTypeWins(0)
        {}

        EconTeam(SerializedGameData& sgd, unsigned numGoodTypesToCollect);

        void Serialize(SerializedGameData& sgd) const;

        // Returns true if the player is in the team
        bool containsPlayer(unsigned playerId) const;
    };

private:
    /// Frame in which the game is going to end
    unsigned endFrame;
    /// Good types to collect
    std::vector<GoodType> goodsToCollect;

    // Data for economy mode progress tracking
    std::vector<EconTeam> economyModeTeams;
    // Maximal amounts collected any team separately for each good type to collect
    std::vector<unsigned> maxAmountsATeamCollected;
    // Amounts collected by each player for each good type to collect
    std::vector<std::array<unsigned, MAX_PLAYERS>> amountsThePlayersCollected;

    // Number of Good types the best team is currently leading in
    unsigned mostGoodTypeWins;

    // Gameframe in which the economy mode progress trackingd data has been updated last
    unsigned gfLastUpdated;

    // Sum up all forms of the given good in the inventory (tools, weapons and beer are also counted in the hands of
    // workers and soldiers)
    unsigned SumUpGood(GoodType good, const Inventory& Inventory);

    // Determine the teams for the economy mode
    void DetermineTeams();

public:
    EconomyModeHandler(unsigned endFrame);

    EconomyModeHandler(SerializedGameData& sgd, unsigned objId);

    /// Destroy
    void Destroy() override;

    void Serialize(SerializedGameData& sgd) const override;

    /// Event-Handler
    void HandleEvent(unsigned id) override;

    // Return vector of the teams (and their collected amounts)
    const std::vector<EconTeam>& GetTeams()
    {
        DetermineTeams();
        return economyModeTeams;
    }

    // Method to update the ware trackers
    void UpdateAmounts();

    // Get the amounts collected by a player
    unsigned GetAmount(unsigned goodNumber, unsigned playerId)
    {
        return amountsThePlayersCollected[goodNumber][playerId];
    }

    // Get the amount of good the leading team (with regards to that good) has collected
    unsigned GetMaxTeamAmount(unsigned goodNumber) { return maxAmountsATeamCollected[goodNumber]; }

    // Get Game frame in which the economy mode winners will be determined
    unsigned GetEndFrame() const { return endFrame; }

    // Return the good types to collect
    const std::vector<GoodType>& GetGoodTypesToCollect() const { return goodsToCollect; }

    GO_Type GetGOT() const override { return GOT_ECONOMYMODEHANDLER; }

    // Miscellaneous status checks
    bool isOver() const;
    bool isInfinite() const { return endFrame == 0; }
    bool showAllTeamAmounts() const { return isOver() || isInfinite(); }
};
