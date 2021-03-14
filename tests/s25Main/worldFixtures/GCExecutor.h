// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
