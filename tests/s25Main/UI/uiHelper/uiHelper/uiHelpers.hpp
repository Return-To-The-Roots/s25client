// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

class MockupVideoDriver;

namespace uiHelper {
/// Initialize data required for GUI tests:
/// Mockup VideoDriver loaded, Dummy files in LOADER, Dummy Desktop activated
void initGUITests();

struct Fixture
{
    Fixture() { initGUITests(); }
};

/// Return the video driver. Initializes the GUI tests if required
MockupVideoDriver* GetVideoDriver();
} // namespace uiHelper
