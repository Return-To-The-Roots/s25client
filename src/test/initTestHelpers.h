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

#ifndef initTestHelpers_h__
#define initTestHelpers_h__

class MockupVideoDriver;

/// Initialize the ingame-Random Number Generator with the given value
/// unless RTTR_RAND_TEST is defined in which case a random value is used
void doInitGameRNG(unsigned defaultValue = 1337, const char* fileName = "", unsigned line = 0);
/// Macro to automatically add file and line info. 2nd form takes a defaultValue
#define initGameRNG() doInitGameRNG(1337, __FILE__, __LINE__);
#define initGameRNG2(defaultValue) doInitGameRNG(defaultValue, __FILE__, __LINE__);
// Would work in C++11
//#define initGameRNG(defaultValue) doInitGameRNG(__VA_ARGS__ + 0, __FILE__, __LINE__);

#define RTTR_REQUIRE_EQUAL_COLLECTIONS(Col1, Col2) BOOST_REQUIRE_EQUAL_COLLECTIONS(Col1.begin(), Col1.end(), Col2.begin(), Col2.end())

/// Initialize data required for GUI tests:
/// Mockup VideoDriver loaded, Dummy files in LOADER, Dummy Desktop activated
void initGUITests();

/// Return the video driver. Initializes the GUI tests if required
MockupVideoDriver* GetVideoDriver();

#endif // initTestHelpers_h__
