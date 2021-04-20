// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DiplomacyPostQuestion.h"
#include "GamePlayerInfo.h"
// TODO: Remove this and add FormatGFTime somewhere else
#include "network/GameClient.h"
#include <boost/format.hpp>

DiplomacyPostQuestion::DiplomacyPostQuestion(unsigned sendFrame, PactType pact, unsigned id,
                                             const GamePlayerInfo& otherPlayer, int duration)
    : PostMsg(sendFrame, "", PostCategory::Diplomacy), acceptOrCancel(true), pact(pact), pactId(id),
      player(otherPlayer.GetPlayerId())
{
    std::string text =
      boost::str(boost::format(_("The player '%s' offers you a %s.")) % otherPlayer.name % _(PACT_NAMES[pact])) + "\n";
    if(duration < 0)
        text += _("Duration: Forever");
    else
        text += boost::str(boost::format(_("Duration: %d GF (%s)")) % duration
                           % GAMECLIENT.FormatGFTime(static_cast<unsigned>(duration)));

    SetText(text);
}

DiplomacyPostQuestion::DiplomacyPostQuestion(unsigned sendFrame, PactType pact, unsigned id,
                                             const GamePlayerInfo& otherPlayer)
    : PostMsg(sendFrame, "", PostCategory::Diplomacy), acceptOrCancel(false), pact(pact), pactId(id),
      player(otherPlayer.GetPlayerId())
{
    std::string text =
      boost::str(boost::format(_("The player '%s' want to cancel the '%s' between you both prematurely. Do you agree?"))
                 % otherPlayer.name % _(PACT_NAMES[pact]));
    SetText(text);
}
