// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>

class RttrConfig;

void convertAndSaveSounds(const RttrConfig& config, const boost::filesystem::path& targetFolder);
