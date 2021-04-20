// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBase.h"

class noNothing final : public noBase
{
public:
    noNothing() : noBase(NodalObjectType::Nothing) {}

    void Destroy() override {}
    GO_Type GetGOT() const final { return GO_Type::Nothing; }
    void Draw(DrawPoint) override {}
};
