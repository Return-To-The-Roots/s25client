// $Id: iwDirectIPConnect.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef iwDIRECTIPCONNECT_H_INCLUDED
#define iwDIRECTIPCONNECT_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "ClientInterface.h"

class iwDirectIPConnect : public IngameWindow, public ClientInterface
{
    private:
        unsigned int server_type;

    public:
        iwDirectIPConnect(unsigned int server_type);
        void SetHost(const char* text);
        void SetPort(unsigned short port);

    private:
        void SetText(const std::string& text, unsigned int color, bool button);

        void Msg_EditChange(const unsigned int ctrl_id);
        void Msg_EditEnter(const unsigned int ctrl_id);
        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection);

        void CI_Error(const ClientError ce);
        void CI_NextConnectState(const ConnectState cs);

};

#endif // !iwDIRECTIPCONNECT_H_INCLUDED
