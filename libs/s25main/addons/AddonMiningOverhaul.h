// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

enum class MiningBehavior
{
    Normal,
    S4Like,
    Inexhaustible,
    AlwaysAvailable
};

class AddonMiningOverhaulBase : public AddonList
{
protected:
    AddonMiningOverhaulBase(AddonId addonId, const std::string& addonName)
        : AddonList(addonId, AddonGroup::Economy, addonName,
                    _("This addon allows you to change the ore mining behavior.\n\n"
                      "No change: Original behavior\n"
                      "Settlers IV: Mines never fully deplete. Range is decreased to 1. Chance for production"
                      " depends on remaining resource amount in range.\n"
                      "Inexhaustible: Mines never deplete\n"
                      "Everywhere: Mines never deplete, can mine everywhere"),
                    {
                      _("No change"),
                      _("Settlers IV"),
                      _("Inexhaustible"),
                      _("Everywhere"),
                    })
    {}
};

class AddonMiningOverhaulCoal : public AddonMiningOverhaulBase
{
public:
    AddonMiningOverhaulCoal() : AddonMiningOverhaulBase(AddonId::MINING_OVERHAUL_COAL, _("Mining overhaul: Coal")) {}
};

class AddonMiningOverhaulGold : public AddonMiningOverhaulBase
{
public:
    AddonMiningOverhaulGold() : AddonMiningOverhaulBase(AddonId::MINING_OVERHAUL_GOLD, _("Mining overhaul: Gold")) {}
};

class AddonMiningOverhaulGranite : public AddonMiningOverhaulBase
{
public:
    AddonMiningOverhaulGranite() : AddonMiningOverhaulBase(AddonId::MINING_OVERHAUL_GRANITE, _("Mining overhaul: Granite")) {}
};

class AddonMiningOverhaulIron : public AddonMiningOverhaulBase
{
public:
    AddonMiningOverhaulIron() : AddonMiningOverhaulBase(AddonId::MINING_OVERHAUL_IRON, _("Mining overhaul: Iron")) {}
};