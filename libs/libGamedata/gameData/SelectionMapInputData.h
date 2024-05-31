// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <boost/filesystem.hpp>
#include <string>
#include <vector>

struct ImageResource
{
    boost::filesystem::path filePath;
    unsigned index;
    ImageResource(boost::filesystem::path path = boost::filesystem::path(), unsigned index = 0)
        : filePath(std::move(path)), index(index){};
};

struct MissionSelectionInfo
{
    ///  The maskAreaColor is the color for the mission used in the missionMapMask
    unsigned maskAreaColor = 0;
    /// The position on which the conquered image and the marker image, if mission is selected,
    /// are displayed. The offset is always counted from the origin of the map image.
    Position ankerPos;
};

struct SelectionMapInputData
{
    SelectionMapInputData() = default;

    /// Background image for the selection map
    ImageResource background;
    /// The map image itself.
    ImageResource map;
    /// This image is a mask that describes the mission areas of the map (image). It must be the same size
    /// as the map image where the color of each pixel determines the mission it belongs to.
    /// Each mission must have a unique color (specified in the missionSelectionInfos).
    /// Any other color is treated as neutral area and ignored.
    ImageResource missionMapMask;
    /// The marker image shown when a mission is selected.
    ImageResource marker;
    /// The image shown when a mission is already finished.
    ImageResource conquered;
    /// Offset of the map image and missionMapMask image relative to the background image.
    Position mapOffsetInBackground = {0, 0};
    /// Color for drawing missions not playable yet. Usually this should be a partly transparent color
    unsigned disabledColor = 0x70000000;
    /// Contains an entry for each mission
    std::vector<MissionSelectionInfo> missionSelectionInfos;
};
