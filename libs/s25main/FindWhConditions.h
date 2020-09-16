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

#pragma once

#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

class nobBaseWarehouse;

/// Vorgefertigte Bedingungsfunktionen für FindWarehouse, param jeweils Pointer auf die einzelnen Strukturen
namespace FW {
struct HasMinWares
{
    const GoodType type;
    const unsigned count;
    HasMinWares(const GoodType type, unsigned count = 1) : type(type), count(count) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct HasFigure
{
    const Job type;
    const bool recruitingAllowed;
    HasFigure(const Job type, bool recruitingAllowed) : type(type), recruitingAllowed(recruitingAllowed) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct HasWareAndFigure : protected HasMinWares, protected HasFigure
{
    HasWareAndFigure(const GoodType good, const Job job, bool recruitingAllowed)
        : HasMinWares(good, 1), HasFigure(job, recruitingAllowed)
    {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct HasMinSoldiers
{
    const unsigned count;
    HasMinSoldiers(unsigned count) : count(count) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct AcceptsWare
{
    const GoodType type;
    AcceptsWare(const GoodType type) : type(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct AcceptsFigure
{
    const Job type;
    AcceptsFigure(const Job type) : type(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct CollectsWare
{
    const GoodType type;
    CollectsWare(const GoodType type) : type(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct CollectsFigure
{
    const Job type;
    CollectsFigure(const Job type) : type(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

// Lagerhäuser enthalten die jeweiligen Waren, liefern sie aber NICHT gleichzeitig ein
struct HasWareButNoCollect : protected HasMinWares, protected CollectsWare
{
    HasWareButNoCollect(const GoodType type) : HasMinWares(type, 1), CollectsWare(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct HasFigureButNoCollect : protected HasFigure, protected CollectsFigure
{
    HasFigureButNoCollect(const Job type, bool recruitingAllowed)
        : HasFigure(type, recruitingAllowed), CollectsFigure(type)
    {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct AcceptsWareButNoSend : protected AcceptsWare
{
    AcceptsWareButNoSend(const GoodType type) : AcceptsWare(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct AcceptsFigureButNoSend : protected AcceptsFigure
{
    AcceptsFigureButNoSend(const Job type) : AcceptsFigure(type) {}
    bool operator()(const nobBaseWarehouse& wh) const;
};

struct NoCondition
{
    bool operator()(const nobBaseWarehouse& /*wh*/) const { return true; }
};
} // namespace FW
