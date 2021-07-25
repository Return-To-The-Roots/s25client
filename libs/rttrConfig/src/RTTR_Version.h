// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace rttr { namespace version {
    std::string GetTitle();
    std::string GetVersion();
    std::string GetBuildDate();
    std::string GetRevision();
    std::string GetShortRevision();
    std::string GetYear();
    std::string GetReadableVersion();
}} // namespace rttr::version
