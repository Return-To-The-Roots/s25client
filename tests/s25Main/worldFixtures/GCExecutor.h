// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GameCommand.h"
#include "factories/GameCommandFactory.h"
#include "s25util/Serializer.h"
#include <boost/test/unit_test.hpp>

class GCExecutor : public GameCommandFactory
{
public:
    unsigned curPlayer;
    GCExecutor() : curPlayer(0) {}

protected:
    bool AddGC(gc::GameCommandPtr gc) override
    {
        // Go through serialization to check if that works too
        Serializer ser;
        gc->Serialize(ser);
        gc.reset();
        gc = gc::GameCommand::Deserialize(ser);
        BOOST_TEST_REQUIRE(ser.GetBytesLeft() == 0u);
        Serializer ser2;
        gc->Serialize(ser2);
        BOOST_TEST_REQUIRE(ser2.GetLength() == ser.GetLength());
        BOOST_TEST_REQUIRE(memcmp(ser2.GetData(), ser.GetData(), ser.GetLength()) == 0);
        gc->Execute(GetWorld(), curPlayer);
        return true;
    }

    virtual GameWorld& GetWorld() = 0;
};
