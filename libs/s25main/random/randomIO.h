// Copyright (c) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Random.h"
#include <boost/filesystem/path.hpp>
#include <iosfwd>

std::ostream& operator<<(std::ostream& os, const RandomEntry& entry);

/// Save the log to a file
void saveRandomLog(const boost::filesystem::path& filepath, const std::vector<RandomEntry>& log);
