// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace rttr::version {
std::string getTitle();
std::string getVersion();
std::string getBuildDate();
std::string getRevision();
std::string getShortRevision();
std::string getYear();
std::string getReadableVersion();
} // namespace rttr::version
