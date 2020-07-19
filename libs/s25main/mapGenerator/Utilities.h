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

#ifndef Utilities_h__
#define Utilities_h__

#include "RttrForeachPt.h"
#include "world/NodeMapBase.h"
#include <iostream>

namespace rttr { namespace mapGenerator {

    template<typename T_Value>
    unsigned MaximumDigits(const NodeMapBase<T_Value>& values, const MapPoint& pt, unsigned digits)
    {
        const auto value = static_cast<unsigned>(values[pt]);

        if(value > 99)
        {
            return 3;
        }

        if(value > 9 && digits == 1)
        {
            return 2;
        }

        return digits;
    }

    template<typename T_Value, class T_Container>
    unsigned GetMaximumDigits(const NodeMapBase<T_Value>& values, const T_Container& area)
    {
        unsigned digits = 1;

        for(const MapPoint& pt : area)
        {
            digits = MaximumDigits(values, pt, digits);
        }

        std::cout << "Maximum number of digits: " << digits << std::endl;

        return digits;
    }

    template<typename T_Value>
    unsigned GetMaximumDigits(const NodeMapBase<T_Value>& values)
    {
        unsigned digits = 1;

        RTTR_FOREACH_PT(MapPoint, values.GetSize())
        {
            digits = MaximumDigits(values, pt, digits);
        }

        std::cout << "Maximum number of digits: " << digits << std::endl;

        return digits;
    }

    template<typename T_Value>
    void PrintValue(T_Value value, unsigned digits)
    {
        if(digits == 1)
        {
            std::cout << " " << unsigned(value);
        }

        if(digits == 2)
        {
            if(value < 10)
            {
                std::cout << "  " << unsigned(value);
            } else
            {
                std::cout << " " << unsigned(value);
            }
        }

        if(digits == 3)
        {
            if(value < 10)
            {
                std::cout << "   " << unsigned(value);
            } else if(value < 100)
            {
                std::cout << "  " << unsigned(value);
            } else
            {
                std::cout << " " << unsigned(value);
            }
        }
    }

    template<typename T_Value>
    void Print(const NodeMapBase<T_Value>& values)
    {
        unsigned digits = GetMaximumDigits(values);

        for(unsigned y = 0; y < values.GetHeight(); y++)
        {
            for(unsigned x = 0; x < values.GetWidth(); x++)
            {
                PrintValue(static_cast<unsigned>(values[MapPoint(x, y)]), digits);
            }

            std::cout << std::endl;
        }
    }

    template<typename T_Value, class T_Container>
    void Print(const NodeMapBase<T_Value>& values, const T_Container& area)
    {
        unsigned digits = GetMaximumDigits(values, area);

        for(unsigned iy = 0; iy < values.GetHeight() / 64; iy++)
        {
            for(unsigned ix = 0; ix < values.GetWidth() / 64; ix++)
            {
                std::cout << std::endl;
                std::cout << "Block (" << ix << "/" << iy << ")" << std::endl;

                for(unsigned y = iy * 64; y < (iy + 1) * 64; y++)
                {
                    for(unsigned x = ix * 64; x < (ix + 1) * 64; x++)
                    {
                        MapPoint pt(x, y);
                        if(helpers::contains(area, pt))
                        {
                            PrintValue(values[pt], digits);
                        } else
                        {
                            if(digits == 1)
                            {
                                std::cout << " x";
                            }

                            if(digits == 2)
                            {
                                std::cout << "  x";
                            }

                            if(digits == 3)
                            {
                                std::cout << "   x";
                            }
                        }
                    }

                    std::cout << std::endl;
                }
            }
        }
    }

}} // namespace rttr::mapGenerator

#endif // Utilities_h__
