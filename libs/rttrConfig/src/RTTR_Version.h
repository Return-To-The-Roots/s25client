// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

class RTTR_Version
{
public:
    static std::string GetTitle();
    static std::string GetVersionDate();
    static std::string GetRevision();
    static std::string GetShortRevision();
    static std::string GetYear();
    static std::string GetReadableVersion();
};
