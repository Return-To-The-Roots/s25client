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

#ifndef POSTMSG_H_
#define POSTMSG_H_

#include "gameTypes/MapTypes.h"
#include "gameTypes/MessageTypes.h"
#include "gameTypes/PactTypes.h"
#include <string>

struct BasePlayerInfo;
class glArchivItem_Bitmap;

class PostMsg
{
public:
    /// Create a simple message
    PostMsg(unsigned sendFrame, const std::string& text, PostMessageCategory cat, const MapPoint& pt = MapPoint::Invalid());
    /// Reponse to a diplomacy question. Last parameter states if the pact was accepted(true) or canceled(false)
    PostMsg(unsigned sendFrame, PactType pt, const BasePlayerInfo& otherPlayer, bool acceptedOrCanceled);
    virtual ~PostMsg(){}

    unsigned GetSendFrame() const { return sendFrame_; }
    const std::string& GetText() const { return text_; }
    PostMessageCategory GetCategory() const { return cat_; }
    /// Get position related to this message (optional, defaults to invalid)
    MapPoint GetPos() const { return pt_; }
    /// Get Associated image or NULL if none exists
    virtual glArchivItem_Bitmap* GetImage_() const { return NULL; }

protected:
    void SetText(const std::string& text){ text_ = text; }

private:
    unsigned sendFrame_;
    std::string text_;
    PostMessageCategory cat_;
    MapPoint pt_;
};

#endif
