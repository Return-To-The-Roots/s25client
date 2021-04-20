// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
