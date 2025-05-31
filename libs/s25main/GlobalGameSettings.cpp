// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GlobalGameSettings.h"
#include "Settings.h"
#include "addons/Addon.h"
#include "addons/Addons.h"
#include "helpers/containerUtils.h"
#include "helpers/serializeEnums.h"
#include "gameData/MilitaryConsts.h"
#include "s25util/Log.h"
#include "s25util/Serializer.h"
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>

GlobalGameSettings::GlobalGameSettings()
    : speed(GameSpeed::Normal), objective(GameObjective::None), startWares(StartWares::Normal), lockedTeams(false),
      exploration(Exploration::FogOfWar), teamView(true), randomStartPosition(false)
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

GlobalGameSettings::GlobalGameSettings(GlobalGameSettings&&) noexcept = default;

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

GlobalGameSettings& GlobalGameSettings::operator=(GlobalGameSettings&&) noexcept = default;

GlobalGameSettings::~GlobalGameSettings() = default;

void GlobalGameSettings::registerAllAddons()
{
    // clang-format off
    using AllAddons = boost::mp11::mp_list<
        AddonAdjustMilitaryStrength,
        AddonAIDebugWindow,
        AddonBattlefieldPromotion,
        AddonBurnDuration,
        AddonCatapultGraphics,
        AddonCatapultsAttackAllies,
        AddonChangeGoldDeposits,
        AddonCharburner,
        AddonCoinsCapturedBld,
        AddonCustomBuildSequence,
        AddonDefenderBehavior,
        AddonDemolishBldWORes,
        AddonDemolitionProhibition,
        AddonDurableGeologistSigns,
        AddonEconomyModeGameLength,
        AddonExhaustibleWater,
        AddonFrontierDistanceReachable,
        AddonHalfCostMilEquip,
        AddonInexhaustibleFish,
        AddonInexhaustibleGraniteMines,
        AddonInexhaustibleMines,
        AddonLimitCatapults,
        AddonManualRoadEnlargement,
        AddonMaxRank,
        AddonMaxWaterwayLength,
        AddonMetalworksBehaviorOnZero,
        AddonMilitaryAid,
        AddonMilitaryControl,
        AddonMilitaryHitpoints,
        AddonMoreAnimals,
        AddonNoAlliedPush,
        AddonNoCoinsDefault,
        AddonNumScoutsExploration,
        AddonPeacefulMode,
        AddonRefundMaterials,
        AddonRefundOnEmergency,
        AddonSeaAttack,
        AddonShipSpeed,
        AddonStatisticsVisibility,
        AddonToolOrdering,
        AddonTrade,
        AddonAutoFlags,
        AddonWine
    >;
    // clang-format on
    using namespace boost::mp11;
    mp_for_each<mp_transform<mp_identity, AllAddons>>([this](auto addonType) {
        using AddonType = typename decltype(addonType)::type;
        this->registerAddon(std::make_unique<AddonType>());
    });
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

    helpers::pushEnum<uint8_t>(ser, speed);
    helpers::pushEnum<uint8_t>(ser, objective);
    helpers::pushEnum<uint8_t>(ser, startWares);
    ser.PushBool(lockedTeams);
    helpers::pushEnum<uint8_t>(ser, exploration);
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
    speed = helpers::popEnum<GameSpeed>(ser);
    objective = helpers::popEnum<GameObjective>(ser);
    startWares = helpers::popEnum<StartWares>(ser);
    lockedTeams = ser.PopBool();
    exploration = helpers::popEnum<Exploration>(ser);
    teamView = ser.PopBool();
    randomStartPosition = ser.PopBool();

    unsigned count = ser.PopUnsignedInt();

    resetAddons();

    for(unsigned i = 0; i < count; ++i)
    {
        auto addon = static_cast<AddonId>(ser.PopUnsignedInt());
        unsigned status = ser.PopUnsignedInt();
        setSelection(addon, status);
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
