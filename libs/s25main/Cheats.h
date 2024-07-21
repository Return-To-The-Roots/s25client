// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"
#include <memory>
#include <string>
#include <unordered_set>

class CheatCommandTracker;
class GamePlayer;
class GameWorldBase;
struct KeyEvent;

class Cheats
{
public:
    Cheats(GameWorldBase& world);
    ~Cheats(); // = default - for unique_ptr

    /** In single player games, tracks keyboard events related to cheats.
     * Delegates this responsibility to CheatCommandTracker, which triggers the actual cheats.
     * In multiplayer games, does nothing. No cheats can be triggered in multiplayer.
     *
     * @param ke - The keyboard event encountered.
     */
    void trackKeyEvent(const KeyEvent& ke);

    /** In single player games, tracks chat commands related to cheats.
     * Delegates this responsibility to CheatCommandTracker, which triggers the actual cheats.
     * In multiplayer games, does nothing. No cheats can be triggered in multiplayer.
     *
     * @param cmd - The chat command to track.
     */
    void trackChatCommand(const std::string& cmd);

    /** Toggles cheat mode on and off.
     * Cheat mode needs to be on for any cheats to trigger.
     */
    void toggleCheatMode();
    /** Used by clients to check if cheat mode is on (e.g. to draw special sprites or enable or all buildings).
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

    /** The classic build headquarters cheat.
     * This function is used to check if the cheat building can be placed at the chosen point when opening the activity
     * window.
     *
     * @param mp - The map point where the user clicked to open the activity window.
     * @return true if the building can be placed, false otherwise
     */
    bool canPlaceCheatBuilding(const MapPoint& mp) const;
    /** The classic build headquarters cheat.
     * This function is used to place the cheat building at the chosen point.
     * The building is immediately fully built, there is no need for a building site.
     *
     * @param mp - The map point at which to place the building.
     * @param player - The player to whom the building should belong.
     */
    void placeCheatBuilding(const MapPoint& mp, const GamePlayer& player);

    /** The classic ALT+1 through ALT+6 cheat which changes the game speed.
     *
     * @param speedIndex - 0 is normal, 1 is faster, 2 is even faster, etc.
     */
    void setGameSpeed(uint8_t speedIndex);

    // RTTR cheats

    /** Shares control of the (human) user's country with the AI. Both the user and the AI retain full control of the
     * country, so the user can observe what the AI does or "cooperate" with it.
     */
    void toggleHumanAIPlayer();

    void armageddon();

    enum class ResourceRevealMode
    {
        Nothing,
        Ores,
        Fish,
        Water
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
    /** Destroys all buildings of AI players.
     */
    void destroyAllAIBuildings();

private:
    /** Checks if cheats can be turned on at all.
     *
     * @return true if if cheats can be turned on, false otherwise
     */
    bool canCheatModeBeOn() const;

    std::unique_ptr<CheatCommandTracker> cheatCmdTracker_;
    bool isCheatModeOn_ = false;
    bool isAllVisible_ = false;
    GameWorldBase& world_;
    ResourceRevealMode resourceRevealMode_ = ResourceRevealMode::Nothing;
};
