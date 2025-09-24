// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GoodTypes.h"
#include "JobTypes.h"
#include "helpers/EnumArray.h"

/// Combined array for goods and people with typed accessors
template<typename T>
struct GoodsAndPeopleArray
{
    helpers::EnumArray<T, GoodType> goods = {};
    helpers::EnumArray<T, Job> people = {};
    helpers::EnumArray<T, ArmoredSoldier> armoredSoldiers = {};

    const T& operator[](GoodType good) const { return goods[good]; }
    T& operator[](GoodType good) { return goods[good]; }
    const T& operator[](Job job) const { return people[job]; }
    T& operator[](Job job) { return people[job]; }
    const T& operator[](ArmoredSoldier soldier) const { return armoredSoldiers[soldier]; }
    T& operator[](ArmoredSoldier soldier) { return armoredSoldiers[soldier]; }
};
