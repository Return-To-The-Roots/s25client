// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LanDiscoveryCfg.h"

static LANDiscoveryBase::Config makeDiscoveryConfig()
{
    LANDiscoveryBase::Config cfg;
    cfg.magicQuery = LANDiscoveryBase::Config::MakeMagic("RTTRQRY");
    cfg.magicResponse = LANDiscoveryBase::Config::MakeMagic("RTTRRES");
    cfg.version = 4;
    cfg.portQuery = 3666;
    cfg.portResponse = 3667;
    return cfg;
}

const LANDiscoveryBase::Config LAN_DISCOVERY_CFG = makeDiscoveryConfig();
