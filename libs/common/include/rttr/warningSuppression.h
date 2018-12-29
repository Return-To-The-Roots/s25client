// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef warningSuppression_h__
#define warningSuppression_h__

// Macro that can be used to suppress unused warnings. Required e.g. for const boost::arrays defined in headers
// Don't use this if not absolutely necessary!
#ifdef __GNUC__
#define SUPPRESS_UNUSED __attribute__((unused))
#else
#define SUPPRESS_UNUSED
#endif

/// Silence unused variable warning
#define RTTR_UNUSED(x) (void)(x)

#endif // warningSuppression_h__