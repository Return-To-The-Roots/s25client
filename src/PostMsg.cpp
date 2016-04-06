// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "PostMsg.h"
#include "GameClient.h"
#include "SerializedGameData.h"
#include "GameClientPlayer.h"
#include "Loader.h"
#include <boost/format.hpp>
#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

PostMsg::PostMsg(const std::string& text, PostMessageCategory cat)
    : text(text), type(PMT_NORMAL), cat(cat), sendFrame(GAMECLIENT.GetGFNumber()) { }

PostMsg::PostMsg(SerializedGameData& sgd)
    : text(sgd.PopString()), type(PostMessageType(sgd.PopUnsignedInt())),
      cat(PostMessageCategory(sgd.PopUnsignedInt())), sendFrame(sgd.PopUnsignedInt()) { }


PostMsg::~PostMsg() { }

void PostMsg::Serialize(SerializedGameData& sgd)
{
    sgd.PushString(text);
    sgd.PushUnsignedInt(type);
    sgd.PushUnsignedInt(cat);
    sgd.PushUnsignedInt(sendFrame);
}

PostMsgWithLocation::PostMsgWithLocation(const std::string& text, PostMessageCategory cat, const MapPoint pt)
    : PostMsg(text, cat), pt(pt) { type = PMT_WITH_LOCATION; }

PostMsgWithLocation::PostMsgWithLocation(SerializedGameData& sgd)
    : PostMsg(sgd), pt(sgd.PopMapPoint()) { }

void PostMsgWithLocation::Serialize(SerializedGameData& sgd)
{
    PostMsg::Serialize(sgd);
    sgd.PushMapPoint(pt);
}

ImagePostMsgWithLocation::ImagePostMsgWithLocation(const std::string& text, PostMessageCategory cat,
        const MapPoint pt, BuildingType senderBuilding, Nation senderNation)
    : PostMsgWithLocation(text, cat, pt), senderBuilding(senderBuilding), senderNation(senderNation)
{
    type = PMT_IMAGE_WITH_LOCATION;
}

ImagePostMsgWithLocation::ImagePostMsgWithLocation(const std::string& text, PostMessageCategory cat,
        const MapPoint pt, Nation senderNation)
    : PostMsgWithLocation(text, cat, pt), senderBuilding(BLD_NOTHING), senderNation(senderNation)
{
    type = PMT_IMAGE_WITH_LOCATION;
}

ImagePostMsgWithLocation::ImagePostMsgWithLocation(SerializedGameData& sgd)
    : PostMsgWithLocation(sgd), senderBuilding(BuildingType(sgd.PopUnsignedInt())), senderNation(Nation(sgd.PopUnsignedInt())) { }

glArchivItem_Bitmap* ImagePostMsgWithLocation::GetImage_() const
{
    if (senderBuilding == BLD_NOTHING)
    {
        // mal sehen, nach messagetype bebildern?
        throw std::runtime_error("Post message without building not supported (yet)");
    }
    return LOADER.GetNationImage(senderNation, 250 + 5 * senderBuilding);
}



void ImagePostMsgWithLocation::Serialize(SerializedGameData& sgd)
{
    PostMsgWithLocation::Serialize(sgd);
    sgd.PushUnsignedInt(senderBuilding);
    sgd.PushUnsignedInt(senderNation);
}

/// Titel für die Fenster für unterschiedliche Bündnistypen
const char* const PACT_TITLES[PACTS_COUNT] =
{
    gettext_noop("treaty of alliance"),
    gettext_noop("non-aggression pact")
};

/// Akzeptieren
DiplomacyPostQuestion::DiplomacyPostQuestion(const unsigned id, const unsigned char player, const PactType pt, const unsigned duration)
    : PostMsg("", PMC_DIPLOMACY), dp_type(ACCEPT), id(id), player(player), pt(pt)
{
    type = PMT_DIPLOMACYQUESTION;
    text = boost::str(
               boost::format(_("The player '%s' offers you a %s.")) % GAMECLIENT.GetPlayer(player).name % _(PACT_TITLES[pt])
           ) + "\n";

    if(duration == 0xFFFFFFFF)
        text += _("Duration: Forever");
    else
        text += boost::str(
                    boost::format(_("Duration: %u GF (%s)")) % duration % GAMECLIENT.FormatGFTime(duration)
                );
}

/// Vertrag auflösen
DiplomacyPostQuestion::DiplomacyPostQuestion(const unsigned id, const unsigned char player, const PactType pt)
    : PostMsg("", PMC_DIPLOMACY), dp_type(CANCEL), id(id), player(player), pt(pt)
{
    type = PMT_DIPLOMACYQUESTION;

    char msg[512];
    sprintf(msg, _("The player '%s' want to cancel the '%s' between you both prematurely. Do you agree?"), GAMECLIENT.GetPlayer(player).name.c_str(),
            _(PACT_TITLES[pt]));

    text = msg;
}

DiplomacyPostQuestion::DiplomacyPostQuestion(SerializedGameData& sgd)
    : PostMsg(sgd), dp_type(Type(sgd.PopUnsignedChar())), id(sgd.PopUnsignedChar()), player(sgd.PopUnsignedChar()), pt(PactType(sgd.PopUnsignedChar()))
{
}

void DiplomacyPostQuestion::Serialize(SerializedGameData& sgd)
{
    PostMsg::Serialize(sgd);
    sgd.PushUnsignedChar(dp_type);
    sgd.PushUnsignedInt(id);
    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedChar(pt);
}


DiplomacyPostInfo::DiplomacyPostInfo(const unsigned char other_player, const Type type, const PactType pt)
    : PostMsg("", PMC_DIPLOMACY)
{
    this->type = PMT_DIPLOMACYINFO;

    char msg[512];
    if(type == ACCEPT)
        sprintf(msg, _("The %s between player '%s' and you has been concluded."), _(PACT_TITLES[pt]),
                GAMECLIENT.GetPlayer(other_player).name.c_str());
    else if(type == CANCEL)
        sprintf(msg, _("The %s between player '%s' and you has been cancelled."), _(PACT_TITLES[pt]),
                GAMECLIENT.GetPlayer(other_player).name.c_str());

    text = msg;
}


DiplomacyPostInfo::DiplomacyPostInfo(SerializedGameData& sgd) : PostMsg(sgd)
{
}


void DiplomacyPostInfo::Serialize(SerializedGameData& sgd)
{
    PostMsg::Serialize(sgd);
}


ShipPostMsg::ShipPostMsg(const std::string& text, PostMessageCategory cat, Nation senderNation, const MapPoint pt)
    :   ImagePostMsgWithLocation(text, cat, pt, senderNation)
{
    type = PMT_SHIP;
}

glArchivItem_Bitmap* ShipPostMsg::GetImage_() const
{
    return LOADER.GetImageN("boot_z", 12 + ((3 + 3) % 6) * 2);
}
