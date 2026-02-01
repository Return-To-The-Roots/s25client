// Copyright (C) 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"
#include <memory>
#include <string>
#include <unordered_set>

class GamePlayer;
class GameWorldBase;
class GameCommandFactory;

class Cheats
{
public:
    Cheats(GameWorldBase& world, GameCommandFactory& gcFactory);

    bool areCheatsAllowed() const;

    /** Toggles cheat mode on and off.
     * Cheat mode needs to be on for any cheats to trigger.
     */
    void toggleCheatMode();
    /** Check if cheat mode is on (e.g. to draw special sprites or enable or all buildings).
     * Cheat mode needs to be on for any cheats to trigger.
     *
     * @return true if cheat mode is on, false otherwise
     */
    bool isCheatModeOn() const { return isCheatModeOn_; }

    // Classic S2 cheats

    /** The classic F7 cheat.
     * Does not modify game state, merely tricks clients into revealing the whole map.
     * In the background, visibility is tracked as expected, i.e. if you reveal the map, send a scout and unreveal the
     * map, you will see what was scouted.
     */
    void toggleAllVisible();
    bool isAllVisible() const { return isAllVisible_; }

    void toggleAllBuildingsEnabled();
    bool areAllBuildingsEnabled() const { return areAllBuildingsEnabled_; }

    void toggleShowEnemyProductivityOverlay();
    bool shouldShowEnemyProductivityOverlay() const { return shouldShowEnemyProductivityOverlay_; }

    /** The classic build headquarters cheat.
     * Check if the cheat building can be placed at the chosen point.
     *
     * @param mp - The map point, e.g. where the user clicked to open the activity window.
     * @return true if the building can be placed, false otherwise
     */
    bool canPlaceCheatBuilding(const MapPoint& mp) const;
    /** The classic build headquarters cheat.
     * Place the cheat HQ building at the chosen point.
     * The building is immediately fully built, there is no need for a building site.
     *
     * @param mp - The map point at which to place the building.
     * @param player - The player to whom the building should belong.
     */
    void placeCheatBuilding(const MapPoint& mp, const GamePlayer& player);

    // RTTR cheats

    /** Shares control of the (human) user's country with the AI. Both the user and the AI retain full control of the
     * country, so the user can observe what the AI does or "cooperate" with it.
     */
    void toggleHumanAIPlayer();

    void armageddon();

    enum class ResourceRevealMode
    {
        // Order is important as each mode includes the previous ones
        Nothing,
        Ores,
        Fish, /// Ores + Fish
        Water /// Ores + Fish + Water
    };
    /** Tells clients which resources to reveal:
     * Nothing - reveal nothing
     * Ores - reveal ores
     * Fish - reveal ores and fish
     * Water - reveal ores, fish and water
     */
    ResourceRevealMode getResourceRevealMode() const;
    void toggleResourceRevealMode();

    using PlayerIDSet = std::unordered_set<unsigned>;
    /** Destroys all buildings of given players, effectively defeating them.
     *
     * @param playerIds - Set of IDs of players.
     */
    void destroyBuildings(const PlayerIDSet& playerIds);
    /// Destroys all buildings of AI players.
    void destroyAllAIBuildings();

private:
    void turnAllCheatsOff();

    bool isCheatModeOn_ = false;
    bool isAllVisible_ = false;
    bool areAllBuildingsEnabled_ = false;
    bool shouldShowEnemyProductivityOverlay_ = false;
    bool isHumanAIPlayer_ = false;
    GameWorldBase& world_;
    GameCommandFactory& gcFactory_;
    ResourceRevealMode resourceRevealMode_ = ResourceRevealMode::Nothing;
};
