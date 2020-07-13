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

#include "DiplomacyPostQuestion.h"
#include "GamePlayerInfo.h"
// TODO: Remove this and add FormatGFTime somewhere else
#include "network/GameClient.h"
#include <boost/format.hpp>

DiplomacyPostQuestion::DiplomacyPostQuestion(unsigned sendFrame, PactType pact, unsigned id, const GamePlayerInfo& otherPlayer,
                                             int duration)
    : PostMsg(sendFrame, "", PostCategory::Diplomacy), acceptOrCancel(true), pact(pact), pactId(id), player(otherPlayer.GetPlayerId())
{
    std::string text = boost::str(boost::format(_("The player '%s' offers you a %s.")) % otherPlayer.name % _(PACT_NAMES[pact])) + "\n";
    if(duration < 0)
        text += _("Duration: Forever");
    else
        text += boost::str(boost::format(_("Duration: %d GF (%s)")) % duration % GAMECLIENT.FormatGFTime(static_cast<unsigned>(duration)));

    SetText(text);
}

DiplomacyPostQuestion::DiplomacyPostQuestion(unsigned sendFrame, PactType pact, unsigned id, const GamePlayerInfo& otherPlayer)
    : PostMsg(sendFrame, "", PostCategory::Diplomacy), acceptOrCancel(false), pact(pact), pactId(id), player(otherPlayer.GetPlayerId())
{
    std::string text = boost::str(boost::format(_("The player '%s' want to cancel the '%s' between you both prematurely. Do you agree?"))
                                  % otherPlayer.name % _(PACT_NAMES[pact]));
    SetText(text);
}
