// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaInterfaceBase.h"
#include <boost/filesystem/path.hpp>

namespace kaguya {
class State;
} // namespace kaguya
struct CampaignDescription;

class CampaignDataLoader : public LuaInterfaceBase
{
public:
    CampaignDataLoader(CampaignDescription& campaignDesc, const boost::filesystem::path& basePath);
    ~CampaignDataLoader() override;

    bool CheckScriptVersion();

    /// Return version of the interface. Changes here reflect breaking changes
    static unsigned GetVersion();

    bool Load();

    static void Register(kaguya::State& state);

private:
    CampaignDescription& campaignDesc_;
    boost::filesystem::path basePath_;
};
