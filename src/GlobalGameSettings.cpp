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
#include "GlobalGameSettings.h"
#include "Settings.h"
#include "addons/Addons.h"
#include "helpers/containerUtils.h"
#include "gameData/MilitaryConsts.h"
#include "libutil/Log.h"
#include "libutil/Serializer.h"
#include <boost/foreach.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>

GlobalGameSettings::GlobalGameSettings()
    : speed(GS_NORMAL), objective(GO_NONE), startWares(SWR_NORMAL), lockedTeams(false), exploration(EXP_FOGOFWAR), teamView(true),
      randomStartPosition(false)
{
    registerAllAddons();
}

GlobalGameSettings::GlobalGameSettings(const GlobalGameSettings& ggs)
    : speed(ggs.speed), objective(ggs.objective), startWares(ggs.startWares), lockedTeams(ggs.lockedTeams), exploration(ggs.exploration),
      teamView(ggs.teamView), randomStartPosition(ggs.randomStartPosition)
{
    registerAllAddons();
    BOOST_FOREACH(const AddonWithState& addon, ggs.addons)
        setSelection(addon.addon->getId(), addon.status);
}

GlobalGameSettings& GlobalGameSettings::operator=(const GlobalGameSettings& ggs)
{
    if(this == &ggs)
        return *this;
    speed = ggs.speed;
    objective = ggs.objective;
    startWares = ggs.startWares;
    lockedTeams = ggs.lockedTeams;
    exploration = ggs.exploration;
    teamView = ggs.teamView;
    randomStartPosition = ggs.randomStartPosition;
    BOOST_FOREACH(const AddonWithState& addon, ggs.addons)
        setSelection(addon.addon->getId(), addon.status);

    return *this;
}

GlobalGameSettings::~GlobalGameSettings()
{
    // clear memory and dont register addons again
    clearAddons();
}

/**
 *  clears the addon memory.
 */
void GlobalGameSettings::clearAddons()
{
    for(AddonContainer::iterator it = addons.begin(); it != addons.end(); ++it)
        delete it->addon;

    addons.clear();
}

void GlobalGameSettings::registerAllAddons()
{
    registerAddon(new AddonLimitCatapults);
    registerAddon(new AddonInexhaustibleMines);
    registerAddon(new AddonRefundMaterials);
    registerAddon(new AddonExhaustibleWater);
    registerAddon(new AddonRefundOnEmergency);
    registerAddon(new AddonManualRoadEnlargement);
    registerAddon(new AddonCatapultGraphics);
    registerAddon(new AddonMetalworksBehaviorOnZero);

    registerAddon(new AddonDemolitionProhibition);
    registerAddon(new AddonCharburner);
    registerAddon(new AddonTrade);

    registerAddon(new AddonChangeGoldDeposits);
    registerAddon(new AddonMaxWaterwayLength);
    registerAddon(new AddonCustomBuildSequence);
    registerAddon(new AddonStatisticsVisibility);

    registerAddon(new AddonDefenderBehavior);
    registerAddon(new AddonAIDebugWindow);

    registerAddon(new AddonNoCoinsDefault);

    registerAddon(new AddonAdjustMilitaryStrength);

    registerAddon(new AddonToolOrdering);

    registerAddon(new AddonMilitaryAid);
    registerAddon(new AddonInexhaustibleGraniteMines);
    registerAddon(new AddonMaxRank);
    registerAddon(new AddonSeaAttack);
    registerAddon(new AddonInexhaustibleFish);

    registerAddon(new AddonShipSpeed);
    registerAddon(new AddonMoreAnimals);
    registerAddon(new AddonBurnDuration);
    registerAddon(new AddonNoAlliedPush);
    registerAddon(new AddonBattlefieldPromotion);
    registerAddon(new AddonHalfCostMilEquip);
    registerAddon(new AddonMilitaryControl);

    registerAddon(new AddonMilitaryHitpoints);

    registerAddon(new AddonNumScoutsExploration);

    registerAddon(new AddonFrontierDistanceReachable);
    registerAddon(new AddonCoinsCapturedBld);
    registerAddon(new AddonDemolishBldWORes);
}

void GlobalGameSettings::resetAddons()
{
    BOOST_FOREACH(AddonWithState& addon, addons)
        addon.status = addon.addon->getDefaultStatus();
}

const Addon* GlobalGameSettings::getAddon(unsigned nr, unsigned& status) const
{
    const Addon* addon = getAddon(nr);
    if(!addon)
        return NULL;

    status = addons[nr].status;
    return addon;
}

const Addon* GlobalGameSettings::getAddon(unsigned nr) const
{
    if(nr >= addons.size())
        return NULL;
    else
        return addons[nr].addon;
}

bool GlobalGameSettings::isEnabled(AddonId id) const
{
    AddonContainer::const_iterator it = std::find(addons.begin(), addons.end(), id);
    return it != addons.end() && it->status != it->addon->getDefaultStatus();
}

unsigned GlobalGameSettings::getSelection(AddonId id) const
{
    AddonContainer::const_iterator it = std::find(addons.begin(), addons.end(), id);
    if(it == addons.end())
        return 0;
    return it->status;
}

void GlobalGameSettings::registerAddon(Addon* addon)
{
    if(!addon)
        return;

    if(helpers::contains(addons, addon->getId()))
        throw std::runtime_error("Addon already registered");

    // Insert sorted
    AddonWithState newItem(addon);
    addons.insert(std::upper_bound(addons.begin(), addons.end(), newItem), newItem);
}

/**
 *  loads the saved addon configuration from the SETTINGS.
 */
void GlobalGameSettings::LoadSettings()
{
    resetAddons();

    for(std::map<unsigned, unsigned>::iterator it = SETTINGS.addons.configuration.begin(); it != SETTINGS.addons.configuration.end(); ++it)
        setSelection((AddonId::type_)it->first, it->second);
}

/**
 *  saves the current addon configuration to the SETTINGS.
 */
void GlobalGameSettings::SaveSettings() const
{
    SETTINGS.addons.configuration.clear();
    BOOST_FOREACH(const AddonWithState& addon, addons)
        SETTINGS.addons.configuration.insert(std::make_pair(addon.addon->getId(), addon.status));
}

/**
 *  saves the current addon configuration to a serializer object.
 */
void GlobalGameSettings::Serialize(Serializer& ser) const
{
    // LOG.writeToFile(">>> Addon Status:\n");

    ser.PushUnsignedChar(static_cast<unsigned char>(speed));
    ser.PushUnsignedChar(static_cast<unsigned char>(objective));
    ser.PushUnsignedChar(static_cast<unsigned char>(startWares));
    ser.PushBool(lockedTeams);
    ser.PushUnsignedChar(static_cast<unsigned char>(exploration));
    ser.PushBool(teamView);
    ser.PushBool(randomStartPosition);

    ser.PushUnsignedInt(addons.size());
    BOOST_FOREACH(const AddonWithState& addon, addons)
    {
        ser.PushUnsignedInt(addon.addon->getId());
        ser.PushUnsignedInt(addon.status);

        // LOG.writeToFile("\t0x%08X=%d\n") % AddonId::type_(it->addon->getId()) % it->status;
    }
}

/**
 *  reads the current addon configuration from a serializer object.
 */
void GlobalGameSettings::Deserialize(Serializer& ser)
{
    speed = static_cast<GameSpeed>(ser.PopUnsignedChar());
    objective = static_cast<GameObjective>(ser.PopUnsignedChar());
    startWares = static_cast<StartWares>(ser.PopUnsignedChar());
    lockedTeams = ser.PopBool();
    exploration = static_cast<Exploration>(ser.PopUnsignedChar());
    teamView = ser.PopBool();
    randomStartPosition = ser.PopBool();

    unsigned count = ser.PopUnsignedInt();

    resetAddons();

    // LOG.writeToFile("<<< Addon Status:\n");

    for(unsigned i = 0; i < count; ++i)
    {
        AddonId addon = AddonId::type_(ser.PopUnsignedInt());
        unsigned status = ser.PopUnsignedInt();
        setSelection(addon, status);

        // LOG.writeToFile("\t0x%08X=%d\n") % AddonId::type_(addon) % status;
    }
}

void GlobalGameSettings::setSelection(AddonId id, unsigned selection)
{
    AddonContainer::iterator it = std::find(addons.begin(), addons.end(), id);
    if(it == addons.end())
        LOG.write(_("Addon %1$#x not found!\n"), LogTarget::FileAndStderr) % id;
    else
        it->status = selection;
}

unsigned GlobalGameSettings::GetMaxMilitaryRank() const
{
    unsigned selection = getSelection(AddonId::MAX_RANK);
    RTTR_Assert(selection <= MAX_MILITARY_RANK);
    return MAX_MILITARY_RANK - selection;
}
unsigned GlobalGameSettings::GetNumScoutsExedition() const
{
    unsigned selection = getSelection(AddonId::NUM_SCOUTS_EXPLORATION);
    return selection + 1;
}

GlobalGameSettings::AddonWithState::AddonWithState(Addon* addon) : addon(addon), status(addon->getDefaultStatus()) {}

bool GlobalGameSettings::AddonWithState::operator<(const AddonWithState& rhs) const
{
    return addon->getName().compare(rhs.addon->getName()) < 0;
}

bool GlobalGameSettings::AddonWithState::operator==(const AddonId& rhs) const
{
    return addon->getId() == rhs;
}
