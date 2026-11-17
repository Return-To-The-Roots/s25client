// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GameCommand.h"
#include "factories/GameCommandFactory.h"
#include <boost/test/unit_test.hpp>

class GCExecutor : public GameCommandFactory
{
public:
    unsigned curPlayer;
    GCExecutor() : curPlayer(0) {}

protected:
    bool addGC(gc::GameCommandPtr gc) override
    {
        // Go through serialization to check if that works too
        gc::Deserializer ser;
        gc->serialize(ser);
        gc.reset();
        gc = gc::GameCommand::deserialize(ser);
        BOOST_TEST_REQUIRE(ser.getBytesLeft() == 0u);
        Serializer ser2;
        gc->serialize(ser2);
        BOOST_TEST_REQUIRE(ser2.getLength() == ser.getLength());
        BOOST_TEST_REQUIRE(memcmp(ser2.getData(), ser.getData(), ser.getLength()) == 0);
        gc->execute(getWorld(), curPlayer);
        return true;
    }

    /// Constructs a road connecting 2 buildings and checks for success
    void buildRoadForBlds(MapPoint bldPosFrom, MapPoint bldPosTo);

    virtual GameWorld& getWorld() = 0;
};
