// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "gameTypes/ServerType.h"
#include <boost/signals2/connection.hpp>

class iwDirectIPConnect : public IngameWindow
{
private:
    ServerType serverType_;
    boost::signals2::scoped_connection onErrorConnection_;

public:
    iwDirectIPConnect(ServerType serverType);
    void SetHost(const std::string& hostIp);
    void SetPort(unsigned short port);
    /// Connects to the given server or fills in the info if it has a password
    void Connect(const std::string& hostOrIp, unsigned short port, bool isIPv6, bool hasPwd);

private:
    void SetStatus(const std::string& text, unsigned color);

    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
};
