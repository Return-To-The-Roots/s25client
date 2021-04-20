// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/NodeMapUtilities.h"

namespace rttr { namespace mapGenerator {

    std::vector<MapPoint> SelectPoints(const std::function<bool(const MapPoint&)>& predicate, const MapExtent& size)
    {
        std::vector<MapPoint> selectedNodes;
        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(predicate(pt))
            {
                selectedNodes.push_back(pt);
            }
        }
        return selectedNodes;
    }

}} // namespace rttr::mapGenerator
