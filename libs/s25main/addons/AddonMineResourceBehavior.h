// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "const_addons.h"
#include "mygettext/mygettext.h"
#include "gameTypes/MineResourceBehavior.h"
#include <string>

class AddonMineResourceBehaviorBase : public AddonList
{
protected:
    AddonMineResourceBehaviorBase(AddonId id, const std::string& name, const std::string& description)
        : AddonList(id, AddonGroup::Economy, name, description,
                    {_("Default"), _("S4-like exhaustion"), _("Inexhaustible"), _("Work everywhere")},
                    static_cast<unsigned>(MineResourceBehavior::Default))
    {}
};

class AddonCoalMineResourceBehavior : public AddonMineResourceBehaviorBase
{
public:
    AddonCoalMineResourceBehavior()
        : AddonMineResourceBehaviorBase(AddonId::COALMINE_RESOURCE_BEHAVIOR, _("Coal Mine Resource Behavior"),
                                        _("Configures how coal mines consume and exhaust coal deposits."))
    {}
};

class AddonIronMineResourceBehavior : public AddonMineResourceBehaviorBase
{
public:
    AddonIronMineResourceBehavior()
        : AddonMineResourceBehaviorBase(AddonId::IRONMINE_RESOURCE_BEHAVIOR, _("Iron Mine Resource Behavior"),
                                        _("Configures how iron mines consume and exhaust iron deposits."))
    {}
};

class AddonGoldMineResourceBehavior : public AddonMineResourceBehaviorBase
{
public:
    AddonGoldMineResourceBehavior()
        : AddonMineResourceBehaviorBase(AddonId::GOLDMINE_RESOURCE_BEHAVIOR, _("Gold Mine Resource Behavior"),
                                        _("Configures how gold mines consume and exhaust gold deposits."))
    {}
};

class AddonGraniteMineResourceBehavior : public AddonMineResourceBehaviorBase
{
public:
    AddonGraniteMineResourceBehavior()
        : AddonMineResourceBehaviorBase(AddonId::GRANITEMINE_RESOURCE_BEHAVIOR, _("Granite Mine Resource Behavior"),
                                        _("Configures how granite mines consume and exhaust stone deposits."))
    {}
};
