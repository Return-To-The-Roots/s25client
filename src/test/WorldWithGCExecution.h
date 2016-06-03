// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "EmptyWorldFixture.h"
#include "GamePlayer.h"
#include "factories/GameCommandFactory.h"
#include "libutil/src/Serializer.h"

#ifndef WorldWithGCExecution_h__
#define WorldWithGCExecution_h__

template<unsigned T_numPlayers>
class WorldWithGCExecution: public EmptyWorldFixture<T_numPlayers>, public GameCommandFactory
{
public:
    using EmptyWorldFixture<T_numPlayers>::world;

    unsigned curPlayer;
    MapPoint hqPos;
    WorldWithGCExecution(): curPlayer(0), hqPos(world.GetPlayer(curPlayer).GetHQPos()){}
protected:
    bool AddGC(gc::GameCommand* gc) override
    {
        // Go through serialization to check if that works too
        Serializer ser;
        const gc::Type type = gc->GetType();
        gc->Serialize(ser);
        deletePtr(gc);
        gc = gc::GameCommand::Deserialize(type, ser);
        BOOST_REQUIRE_EQUAL(ser.GetBytesLeft(), 0u);
        Serializer ser2;
        gc->Serialize(ser2);
        BOOST_REQUIRE_EQUAL(ser2.GetLength(), ser.GetLength());
        for(unsigned i = 0; i < ser.GetLength(); i++)
            BOOST_REQUIRE_EQUAL(ser2.GetData()[i], ser.GetData()[i]);
        gc->Execute(world, curPlayer);
        deletePtr(gc);
        return true;
    }
};

// Avoid having to use "this->" to access those
class WorldWithGCExecution2P: public WorldWithGCExecution<2>
{
public:
    using WorldWithGCExecution<2>::world;
    using WorldWithGCExecution<2>::curPlayer;
    using WorldWithGCExecution<2>::hqPos;
};

class WorldWithGCExecution3P: public WorldWithGCExecution<3>
{
public:
    using WorldWithGCExecution<3>::world;
    using WorldWithGCExecution<3>::curPlayer;
    using WorldWithGCExecution<3>::hqPos;
};

#endif // WorldWithGCExecution_h__
