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

#include "GlobalGameSettings.h"
#include "Settings.h"
#include "addons/Addon.h"
#include "addons/Addons.h"
#include "helpers/containerUtils.h"
#include "gameData/MilitaryConsts.h"
#include "s25util/Log.h"
#include "s25util/Serializer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

GlobalGameSettings::GlobalGameSettings()
    : speed(GS_NORMAL), objective(GO_NONE), startWares(SWR_NORMAL), lockedTeams(false), exploration(EXP_FOGOFWAR),
      teamView(true), randomStartPosition(false)
{
    registerAllAddons();
}

GlobalGameSettings::GlobalGameSettings(const GlobalGameSettings& ggs)
    : speed(ggs.speed), objective(ggs.objective), startWares(ggs.startWares), lockedTeams(ggs.lockedTeams),
      exploration(ggs.exploration), teamView(ggs.teamView), randomStartPosition(ggs.randomStartPosition)
{
    registerAllAddons();
    for(const AddonWithState& addon : ggs.addons)
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
    for(const AddonWithState& addon : ggs.addons)
        setSelection(addon.addon->getId(), addon.status);

    return *this;
}

GlobalGameSettings::~GlobalGameSettings() = default;

void GlobalGameSettings::registerAllAddons()
{
    registerAddon(std::make_unique<AddonLimitCatapults>());
    registerAddon(std::make_unique<AddonInexhaustibleMines>());
    registerAddon(std::make_unique<AddonRefundMaterials>());
    registerAddon(std::make_unique<AddonExhaustibleWater>());
    registerAddon(std::make_unique<AddonRefundOnEmergency>());
    registerAddon(std::make_unique<AddonManualRoadEnlargement>());
    registerAddon(std::make_unique<AddonCatapultGraphics>());
    registerAddon(std::make_unique<AddonMetalworksBehaviorOnZero>());

    registerAddon(std::make_unique<AddonDemolitionProhibition>());
    registerAddon(std::make_unique<AddonCharburner>());
    registerAddon(std::make_unique<AddonTrade>());

    registerAddon(std::make_unique<AddonChangeGoldDeposits>());
    registerAddon(std::make_unique<AddonMaxWaterwayLength>());
    registerAddon(std::make_unique<AddonCustomBuildSequence>());
    registerAddon(std::make_unique<AddonStatisticsVisibility>());

    registerAddon(std::make_unique<AddonDefenderBehavior>());
    registerAddon(std::make_unique<AddonAIDebugWindow>());

    registerAddon(std::make_unique<AddonNoCoinsDefault>());

    registerAddon(std::make_unique<AddonAdjustMilitaryStrength>());

    registerAddon(std::make_unique<AddonToolOrdering>());

    registerAddon(std::make_unique<AddonMilitaryAid>());
    registerAddon(std::make_unique<AddonInexhaustibleGraniteMines>());
    registerAddon(std::make_unique<AddonMaxRank>());
    registerAddon(std::make_unique<AddonSeaAttack>());
    registerAddon(std::make_unique<AddonInexhaustibleFish>());

    registerAddon(std::make_unique<AddonShipSpeed>());
    registerAddon(std::make_unique<AddonMoreAnimals>());
    registerAddon(std::make_unique<AddonBurnDuration>());
    registerAddon(std::make_unique<AddonNoAlliedPush>());
    registerAddon(std::make_unique<AddonBattlefieldPromotion>());
    registerAddon(std::make_unique<AddonHalfCostMilEquip>());
    registerAddon(std::make_unique<AddonMilitaryControl>());

    registerAddon(std::make_unique<AddonMilitaryHitpoints>());

    registerAddon(std::make_unique<AddonNumScoutsExploration>());

    registerAddon(std::make_unique<AddonFrontierDistanceReachable>());
    registerAddon(std::make_unique<AddonCoinsCapturedBld>());
    registerAddon(std::make_unique<AddonDemolishBldWORes>());

    registerAddon(std::make_unique<AddonPeacefulMode>());
    registerAddon(std::make_unique<AddonDurableGeologistSigns>());
}

void GlobalGameSettings::resetAddons()
{
    for(AddonWithState& addon : addons)
        addon.status = addon.addon->getDefaultStatus();
}

const Addon* GlobalGameSettings::getAddon(unsigned idx, unsigned& status) const
{
    const Addon* addon = getAddon(idx);
    if(addon)
        status = addons[idx].status;
    return addon;
}

const Addon* GlobalGameSettings::getAddon(unsigned idx) const
{
    if(idx >= addons.size())
        return nullptr;
    else
        return addons[idx].addon.get();
}

GlobalGameSettings::AddonWithState* GlobalGameSettings::getAddon(AddonId id)
{
    auto it = helpers::find_if(addons, [id](const AddonWithState& cur) { return cur.addon->getId() == id; });
    return it != addons.end() ? &*it : nullptr;
}

const GlobalGameSettings::AddonWithState* GlobalGameSettings::getAddon(AddonId id) const
{
    auto it = helpers::find_if(addons, [id](const AddonWithState& cur) { return cur.addon->getId() == id; });
    return it != addons.end() ? &*it : nullptr;
}

bool GlobalGameSettings::isEnabled(AddonId id) const
{
    const auto* addon = getAddon(id);
    return addon && addon->status != addon->addon->getDefaultStatus();
}

unsigned GlobalGameSettings::getSelection(AddonId id) const
{
    const auto* addon = getAddon(id);
    return addon ? addon->status : 0;
}

void GlobalGameSettings::registerAddon(std::unique_ptr<Addon> addon)
{
    if(!addon)
        return;

    if(getAddon(addon->getId()))
        throw std::runtime_error("Addon already registered");

    // Insert sorted
    AddonWithState newItem(std::move(addon));
    const auto cmpByName = [](const AddonWithState& lhs, const AddonWithState& rhs) {
        return lhs.addon->getName().compare(rhs.addon->getName()) < 0;
    };
    addons.insert(std::upper_bound(addons.begin(), addons.end(), newItem, cmpByName), std::move(newItem));
}

/**
 *  loads the saved addon configuration from the SETTINGS.
 */
void GlobalGameSettings::LoadSettings()
{
    resetAddons();

    for(const auto& it : SETTINGS.addons.configuration)
        setSelection(static_cast<AddonId>(it.first), it.second);
}

/**
 *  saves the current addon configuration to the SETTINGS.
 */
void GlobalGameSettings::SaveSettings() const
{
    SETTINGS.addons.configuration.clear();
    for(const AddonWithState& addon : addons)
        SETTINGS.addons.configuration.insert(std::make_pair(static_cast<unsigned>(addon.addon->getId()), addon.status));
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
    for(const AddonWithState& addon : addons)
    {
        ser.PushUnsignedInt(static_cast<unsigned>(addon.addon->getId()));
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
        auto addon = static_cast<AddonId>(ser.PopUnsignedInt());
        unsigned status = ser.PopUnsignedInt();
        setSelection(addon, status);

        // LOG.writeToFile("\t0x%08X=%d\n") % AddonId::type_(addon) % status;
    }
}

void GlobalGameSettings::setSelection(AddonId id, unsigned selection)
{
    auto* addon = getAddon(id);
    if(!addon)
        LOG.write(_("Addon %1$#x not found!\n"), LogTarget::FileAndStderr) % static_cast<unsigned>(id);
    else
        addon->status = selection;
}

unsigned GlobalGameSettings::GetMaxMilitaryRank() const
{
    unsigned selection = getSelection(AddonId::MAX_RANK);
    RTTR_Assert(selection <= MAX_MILITARY_RANK);
    return MAX_MILITARY_RANK - selection;
}
unsigned GlobalGameSettings::GetNumScoutsExpedition() const
{
    unsigned selection = getSelection(AddonId::NUM_SCOUTS_EXPLORATION);
    return selection + 1;
}

GlobalGameSettings::AddonWithState::AddonWithState(std::unique_ptr<Addon> addon)
    : addon(std::move(addon)), status(this->addon->getDefaultStatus())
{}
