// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef ReturnConst_h__
#define ReturnConst_h__

/// Returns the same value of a given type for all invocations
template<typename T_Type, T_Type T_value>
struct ReturnConst
{
    typedef T_Type result_type;

    template<typename T>
    result_type operator()(const T&) const
    {
        return T_value;
    }
};

#endif // ReturnConst_h__


