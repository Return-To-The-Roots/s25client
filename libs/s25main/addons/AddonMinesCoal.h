// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

class AddonMinesCoal : public AddonList
{
public:
    AddonMinesCoal()
        : AddonList(AddonId::MINES_COAL, AddonGroup::Economy, _("Change coal mine behavior"),
                    _("This addon lets you control mining behavior.\n\n"
                      "No change: Original behavior\n"
                      "Settlers IV: Mines never deplete, but mining becomes less successfull\n"
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