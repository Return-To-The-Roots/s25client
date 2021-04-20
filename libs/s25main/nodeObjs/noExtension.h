// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBase.h"
class SerializedGameData;

// Große Gebäude erstrecken sich über 4 Felder, die restlichen 3 werden mit dieser Klasse gefüllt
class noExtension final : public noBase
{
public:
    noExtension(noBase* const base) : noBase(NodalObjectType::Extension), base(base) {}
    noExtension(SerializedGameData& sgd, unsigned obj_id);
    ~noExtension() override;

    noBase* GetBaseObject() const { return base; }
    void Draw(DrawPoint /*drawPt*/) override {}

    void Destroy() override {}
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Extension; }

    BlockingManner GetBM() const override { return BlockingManner::Single; }

private:
    noBase* const base;
};
