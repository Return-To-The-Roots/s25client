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

#ifndef GCExecutor_h__
#define GCExecutor_h__

#include "GameCommand.h"
#include "factories/GameCommandFactory.h"
#include "libutil/Serializer.h"
#include <boost/test/unit_test.hpp>

class GCExecutor : public GameCommandFactory
{
public:
    unsigned curPlayer;
    GCExecutor() : curPlayer(0) {}

protected:
    bool AddGC(gc::GameCommand* gc) override
    {
        // Go through serialization to check if that works too
        Serializer ser;
        gc->Serialize(ser);
        deletePtr(gc);
        gc = gc::GameCommand::Deserialize(ser);
        BOOST_REQUIRE_EQUAL(ser.GetBytesLeft(), 0u);
        Serializer ser2;
        gc->Serialize(ser2);
        BOOST_REQUIRE_EQUAL(ser2.GetLength(), ser.GetLength());
        BOOST_REQUIRE_EQUAL(memcmp(ser2.GetData(), ser.GetData(), ser.GetLength()), 0);
        gc->Execute(GetWorld(), curPlayer);
        deletePtr(gc);
        return true;
    }

    virtual GameWorldGame& GetWorld() = 0;
};

#endif // GCExecutor_h__
