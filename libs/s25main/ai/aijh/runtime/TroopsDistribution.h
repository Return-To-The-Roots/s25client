// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

namespace AIJH {

std::vector<unsigned> ComputeTroopsDistributionLimits(const std::vector<unsigned>& capacities,
                                                      std::vector<double> scores, unsigned distributableSoldiers,
                                                      unsigned startIdx);

} // namespace AIJH
