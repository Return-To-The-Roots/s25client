// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef AIInfo_h__
#define AIInfo_h__

class Serializer;

namespace AI {
enum Level
{
    EASY = 0,
    MEDIUM,
    HARD
};

enum Type
{
    DUMMY = 0,
    DEFAULT
};

struct Info
{
    Type type;
    Level level;
    Info(Type t = DUMMY, Level l = EASY) : type(t), level(l) {}
    Info(Serializer& ser);
    void serialize(Serializer& ser) const;
};
} // namespace AI

#endif // AIInfo_h__
