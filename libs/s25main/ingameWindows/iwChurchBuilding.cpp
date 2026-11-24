// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwChurchBuilding.h"
#include "ChurchLoader.h"
#include "Loader.h"
#include "buildings/nobChurch.h"
#include "controls/ctrlText.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include <boost/format.hpp>

iwChurchBuilding::iwChurchBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* const building)
    : iwBuilding(gwv, gcFactory, building, Extent(226, 266))
{
    getCtrl<Window>(1)->setPos(DrawPoint(117, 126));
    addImage(2, DrawPoint(196, 39), LOADER.getMapTexture(2298));
    addImage(3, DrawPoint(196, 39), churchaddon::getBlessingTex());
    addText(20, DrawPoint(113, 154), "", COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    addText(21, DrawPoint(113, 170), "", COLOR_YELLOW, FontStyle::CENTER, NormalFont);
}

void iwChurchBuilding::msgPaintBefore()
{
    iwBuilding::msgPaintBefore();
    const auto* church = static_cast<const nobChurch*>(building);
    std::string txt = (boost::format(_("Blessings: %u / %u")) % static_cast<unsigned>(church->getNumBlessings())
                       % static_cast<unsigned>(churchaddon::MAX_CHURCH_BLESSINGS))
                        .str();
    switch(church->getServiceState())
    {
        case nobChurch::ServiceState::Gathering: txt += std::string(" - ") + _("Gathering"); break;
        case nobChurch::ServiceState::InService: txt += std::string(" - ") + _("Service!"); break;
        case nobChurch::ServiceState::Idle: break;
    }
    getCtrl<ctrlText>(20)->setText(txt);

    const unsigned soldiersInside = church->getNumSoldiersInside();
    const unsigned soldiersEnRoute = church->getNumSoldiersEnRoute();
    std::string soldierTxt;
    if(soldiersEnRoute > 0)
    {
        soldierTxt = (boost::format(_("Soldiers inside: %u (%u en route)")) % soldiersInside % soldiersEnRoute).str();
    } else
    {
        soldierTxt = (boost::format(_("Soldiers inside: %u")) % soldiersInside).str();
    }
    getCtrl<ctrlText>(21)->setText(soldierTxt);
}
