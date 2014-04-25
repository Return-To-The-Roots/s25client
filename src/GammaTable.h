// $Id: GammaTable.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005-2009 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.
#ifndef GAMMATABLE_H_INCLUDED
#define GAMMATABLE_H_INCLUDED

///////////////////////////////////////////////////////////////////////////////
// Systemheader
#include <math.h>

template <class T> class GammaTable
{
    private:
        unsigned int size;
        float        sizef;
        T*            table;
        float        gamma;

        // ctor
        GammaTable() { }

    public:
        inline const float& get_gamma ()
        {
            return gamma;
        }

        inline void set_gamma (float g)
        {
            if (g < 0.001f) g = 0.001f;
            if (g == gamma) return;
            gamma = g;

            for (unsigned int i = 0; i < size; i++)
                table[i] = (T) (pow (i / sizef, 1 / gamma) * sizef);
        }

        GammaTable (unsigned int s, float g = 1) : sizef(-1), gamma(-1)
        {
            sizef += size = s > 2 ? s : 2;
            table = new T [size];
            set_gamma(g);
        }

        ~GammaTable ()
        {
            delete [] table;
        }

        inline const T& operator [] (const T& i) const
        {
            return table[i];
        }
};

#endif // !GAMMATABLE_H_INCLUDED
