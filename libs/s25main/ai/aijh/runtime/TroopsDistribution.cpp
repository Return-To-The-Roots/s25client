// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/aijh/runtime/TroopsDistribution.h"

#include <algorithm>

namespace AIJH {

std::vector<unsigned> ComputeTroopsDistributionLimits(const std::vector<unsigned>& capacities,
                                                      std::vector<double> scores, const unsigned distributableSoldiers,
                                                      const unsigned startIdx)
{
    if(capacities.empty())
        return {};

    std::vector<unsigned> newLimits(capacities.size(), 1u);
    const bool hasPositiveScore = std::any_of(scores.begin(), scores.end(), [](double score) { return score > 0.0; });
    if(!hasPositiveScore)
        std::fill(scores.begin(), scores.end(), 1.0);

    const unsigned assigned = static_cast<unsigned>(newLimits.size());
    if(distributableSoldiers <= assigned)
        return newLimits;

    unsigned remaining = distributableSoldiers - assigned;

    while(remaining > 0)
    {
        unsigned bestIdx = capacities.size();
        unsigned bestExtraAssigned = 0;
        double bestScore = 1.0;
        unsigned zeroScoreFallbackIdx = capacities.size();

        for(unsigned offset = 0; offset < capacities.size() && remaining > 0; ++offset)
        {
            const unsigned idx = (startIdx + offset) % static_cast<unsigned>(capacities.size());
            const double score = scores[idx];
            if(newLimits[idx] >= capacities[idx])
                continue;
            if(score <= 0.0)
            {
                if(zeroScoreFallbackIdx == capacities.size())
                    zeroScoreFallbackIdx = idx;
                continue;
            }

            const unsigned extraAssigned = newLimits[idx] - 1u;
            if(bestIdx == capacities.size())
            {
                bestIdx = idx;
                bestExtraAssigned = extraAssigned;
                bestScore = score;
                continue;
            }

            const double lhs = static_cast<double>(extraAssigned) * bestScore;
            const double rhs = static_cast<double>(bestExtraAssigned) * score;
            if(lhs < rhs)
            {
                bestIdx = idx;
                bestExtraAssigned = extraAssigned;
                bestScore = score;
            }
        }

        if(bestIdx == capacities.size())
        {
            if(zeroScoreFallbackIdx == capacities.size())
                break;
            bestIdx = zeroScoreFallbackIdx;
        }

        ++newLimits[bestIdx];
        --remaining;
    }

    return newLimits;
}

} // namespace AIJH
