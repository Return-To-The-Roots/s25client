// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdexcept>

class RTTR_AssertError : public std::runtime_error
{
public:
    RTTR_AssertError(const std::string& msg) : std::runtime_error(msg) {}
};
