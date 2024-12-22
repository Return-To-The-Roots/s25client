// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlMapSelection.h"

struct CampaignDescription;

bool isChapterEnabled(const CampaignDescription& campaignDesc, unsigned char chapter);
std::vector<MissionStatus> getMissionsStatus(const CampaignDescription& campaignDesc);
