// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PostMsg.h"
#include "BasePlayerInfo.h"
#include "mygettext/mygettext.h"
#include <boost/format.hpp>
#include <utility>

PostMsg::PostMsg(unsigned sendFrame, std::string text, PostCategory cat, const MapPoint& pt, SoundEffect soundEffect)
    : sendFrame_(sendFrame), text_(std::move(text)), cat_(cat), pt_(pt), soundEffect_(soundEffect)
{}

PostMsg::PostMsg(unsigned sendFrame, std::string text, PostCategory cat, SoundEffect soundEffect)
    : sendFrame_(sendFrame), text_(std::move(text)), cat_(cat), pt_(MapPoint::Invalid()), soundEffect_(soundEffect)
{}

PostMsg::PostMsg(unsigned sendFrame, PactType pt, const BasePlayerInfo& otherPlayer, bool acceptedOrCanceled,
                 SoundEffect soundEffect)
    : sendFrame_(sendFrame), cat_(PostCategory::Diplomacy), pt_(MapPoint::Invalid()), soundEffect_(soundEffect)
{
    if(acceptedOrCanceled)
    {
        text_ = boost::str(boost::format(_("The %s between player '%s' and you has been concluded."))
                           % _(PACT_NAMES[pt]) % otherPlayer.name);
    } else
    {
        text_ = boost::str(boost::format(_("The %s between player '%s' and you has been cancelled."))
                           % _(PACT_NAMES[pt]) % otherPlayer.name);
    }
}
