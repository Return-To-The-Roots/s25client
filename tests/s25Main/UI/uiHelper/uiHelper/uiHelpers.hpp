// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
