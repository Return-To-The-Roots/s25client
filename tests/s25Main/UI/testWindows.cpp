// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GlobalGameSettings.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlTextButton.h"
#include "desktops/Desktop.h"
#include "files.h"
#include "ingameWindows/iwAddonPresets.h"
#include "ingameWindows/iwAddons.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwSkipGFs.h"
#include "ingameWindows/iwVictory.h"
#include "uiHelper/uiHelpers.hpp"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "rttr/test/ConfigOverride.hpp"
#include "rttr/test/TmpFolder.hpp"
#include <turtle/mock.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <mygettext/mygettext.h>
#include <optional>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

using SmallWorldFixture = WorldFixture<CreateEmptyWorld, 1, 10, 10>;

BOOST_FIXTURE_TEST_SUITE(Windows, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(Victory)
{
    std::vector<std::string> winnerNames;
    winnerNames.push_back("FooName");
    winnerNames.push_back("BarNameBaz");
    const iwVictory wnd(winnerNames);
    // 2 buttons
    BOOST_TEST_REQUIRE(wnd.GetCtrls<ctrlButton>().size() == 2u);
    // Find a text field containing all winner names
    const auto txts = wnd.GetCtrls<ctrlMultiline>();
    bool found = false;
    for(const ctrlMultiline* txt : txts)
    {
        if(txt->GetNumLines() != winnerNames.size())
            continue; // LCOV_EXCL_LINE
        bool curFound = true;
        for(unsigned i = 0; i < winnerNames.size(); i++)
        {
            curFound &= txt->GetLine(i) == winnerNames[i];
        }
        found |= curFound;
    }
    BOOST_TEST_REQUIRE(found);
}

BOOST_AUTO_TEST_CASE(AddonWindow)
{
    GlobalGameSettings ggs;
    const iwAddons wndAllChangeable(ggs, nullptr, AddonChangeAllowed::All);
    const iwAddons wndAllReadOnly(ggs, nullptr, AddonChangeAllowed::None);
    const auto addonsChangeableGui = wndAllChangeable.GetCtrls<ctrlGroup>();
    BOOST_TEST_REQUIRE(addonsChangeableGui.size() == ggs.getNumAddons() + 1); // First element is the option group
    const auto addonsReadonlyGui = wndAllReadOnly.GetCtrls<ctrlGroup>();
    BOOST_TEST_REQUIRE(addonsReadonlyGui.size() == ggs.getNumAddons() + 1);
    for(unsigned i = 1; i <= ggs.getNumAddons(); ++i)
    {
        const ctrlGroup* changeableGroup = addonsChangeableGui[i];
        const ctrlGroup* readonlyGroup = addonsReadonlyGui[i];
        // No lock icon
        BOOST_TEST(changeableGroup->GetCtrls<ctrlImage>().empty());
        // Lock icon
        BOOST_TEST_REQUIRE(!readonlyGroup->GetCtrls<ctrlImage>().empty());
        // Verify it is the lock icon with tooltip
        BOOST_TEST_REQUIRE(readonlyGroup->GetCtrls<ctrlImage>()[0]->GetTooltip() == _("Locked"));
        for(const auto* checkbox : changeableGroup->GetCtrls<ctrlCheck>())
            BOOST_TEST_REQUIRE(!checkbox->isReadOnly());
        for(const auto* checkbox : readonlyGroup->GetCtrls<ctrlCheck>())
            BOOST_TEST_REQUIRE(checkbox->isReadOnly());
        for(const auto* cb : changeableGroup->GetCtrls<ctrlComboBox>())
            BOOST_TEST_REQUIRE(!cb->isReadOnly());
        for(const auto* cb : readonlyGroup->GetCtrls<ctrlComboBox>())
            BOOST_TEST_REQUIRE(cb->isReadOnly());
    }
}

BOOST_FIXTURE_TEST_CASE(JumpWindow, SmallWorldFixture)
{
    uiHelper::Fixture f;
    // Test if it is constructible only, accesses GameClient for buttons
    GameWorldViewer gwv(0, world);
    GameWorldView view(gwv, Position(0, 0), Extent(100, 100));
    iwSkipGFs wnd(view);
    // At least 4 buttons for "jump by x" and at least 1 extra for "jump to"
    const auto bts = wnd.GetCtrls<ctrlTextButton>();
    BOOST_TEST(bts.size() > 4);
    const auto numIncBts = helpers::count_if(bts, [](const ctrlTextButton* bt) { return bt->GetText().at(0) == '+'; });
    BOOST_TEST(numIncBts >= 4);
}

namespace {
struct AddonPresetTmpUserData : uiHelper::Fixture
{
    rttr::test::TmpFolder tmp;
    rttr::test::ConfigOverride userDataOverride{"USERDATA", tmp};
};

struct AddonPresetFixture : AddonPresetTmpUserData
{
    // The windows expect the folder to exist, just like when iwAddons opens them
    AddonPresetFixture() { BOOST_TEST_REQUIRE(iwAddonPresetsBase::EnsurePresetsFolder()); }

    // Selects the named preset, false if there is no such preset
    static bool select(Window& wnd, const std::string& name)
    {
        auto& table = *wnd.GetCtrls<ctrlTable>().at(0);
        for(unsigned short i = 0; i < table.GetNumRows(); ++i)
        {
            if(table.GetItemText(i, 0) == name)
            {
                table.SetSelection(i);
                return true;
            }
        }
        return false;
    }

    void save(const std::map<unsigned, unsigned>& states, const std::string& name)
    {
        iwSaveAddonPreset wnd(states);
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText(name);
        base.Msg_ButtonClick(iwAddonPresetsBase::ID_btAction);
    }

    // Returns the given preset's settings, or empty if the preset is missing or corrupt.
    std::map<unsigned, unsigned> load(const std::string& name)
    {
        std::map<unsigned, unsigned> out;
        iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>& s) { out = s; });
        Window& base = wnd;
        if(select(base, name))
            base.Msg_ButtonClick(iwAddonPresetsBase::ID_btAction);
        return out;
    }

    // Presets currently on disk, read via a fresh Load window.
    unsigned numPresets()
    {
        iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
        return wnd.GetCtrls<ctrlTable>().at(0)->GetNumRows();
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(AddonPresetSaveLoadAndOverwrite, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> states1{{1, 2}, {3, 0}};
    const std::map<unsigned, unsigned> states2{{3, 4}};

    // save -> load roundtrip
    save(states1, "myPreset");
    BOOST_TEST(load("myPreset") == states1);

    // overwrite: No - file unchanged
    {
        iwSaveAddonPreset wnd(states2);
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText("myPreset");
        base.Msg_EditEnter(0);

        const auto* msgbox = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
        BOOST_TEST_REQUIRE(msgbox);
        BOOST_TEST(msgbox->GetTitle() == _("Overwrite Preset"));
        bool namesPreset = false;
        for(const auto* ml : msgbox->GetCtrls<ctrlMultiline>())
        {
            for(unsigned i = 0; i < ml->GetNumLines(); ++i)
                namesPreset |= ml->GetLine(i).find("myPreset") != std::string::npos;
        }
        BOOST_TEST(namesPreset);

        base.Msg_MsgBoxResult(iwSaveAddonPreset::ID_mbOverwrite, MsgboxResult::No);
        WINDOWMANAGER.CloseNow(WINDOWMANAGER.GetTopMostWindow()); // free the overwrite prompt
    }
    BOOST_TEST(load("myPreset") == states1); // unchanged

    // overwrite: Yes - file updated
    {
        iwSaveAddonPreset wnd(states2);
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText("myPreset");
        base.Msg_EditEnter(0);
        base.Msg_MsgBoxResult(iwSaveAddonPreset::ID_mbOverwrite, MsgboxResult::Yes);
        WINDOWMANAGER.CloseNow(WINDOWMANAGER.GetTopMostWindow()); // free the overwrite prompt
    }
    BOOST_TEST(load("myPreset") == states2); // updated
}

// Saving a name that already ends in the extension yields a second preset instead of overwriting.
BOOST_FIXTURE_TEST_CASE(AddonPresetExtensionInNameIsDistinct, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> states{{1, 2}};
    const std::map<unsigned, unsigned> statesDoubled{{3, 4}};
    save(states, "myPreset");            // -> myPreset.ini,     listed "myPreset"
    save(statesDoubled, "myPreset.ini"); // -> myPreset.ini.ini, listed "myPreset.ini"
    BOOST_TEST_REQUIRE(numPresets() == 2u);

    BOOST_TEST(load("myPreset") == states);
    BOOST_TEST(load("myPreset.ini") == statesDoubled);
}

BOOST_FIXTURE_TEST_CASE(AddonPresetDoubleClickLoads, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> states{{1, 2}};
    save(states, "myPreset");

    std::optional<std::map<unsigned, unsigned>> loaded;
    iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>& s) { loaded = s; });
    Window& base = wnd;
    BOOST_TEST_REQUIRE(select(base, "myPreset"));
    base.Msg_TableChooseItem(iwAddonPresetsBase::ID_tblPresets, 0u);

    BOOST_TEST_REQUIRE(loaded.has_value());
    BOOST_TEST(*loaded == states);
}

// In the save window the selection only prefills the name: saving uses what is in the edit box.
BOOST_FIXTURE_TEST_CASE(AddonPresetSaveNameFollowsSelection, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> statesA{{1, 2}};
    const std::map<unsigned, unsigned> statesB{{3, 4}};
    const std::map<unsigned, unsigned> statesNew{{5, 6}};
    save(statesA, "presetA");
    save(statesB, "presetB");

    iwSaveAddonPreset wnd(statesNew);
    Window& base = wnd;
    BOOST_TEST(!wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete)); // saving cannot delete
    auto& edit = *wnd.GetCtrls<ctrlEdit>().at(0);
    auto& table = *wnd.GetCtrls<ctrlTable>().at(0);
    // Rows are sorted ascending, so row 0 is presetA
    table.SetSelection(0u);
    BOOST_TEST_REQUIRE(edit.GetText() == "presetA");
    table.SetSelection(1u);
    BOOST_TEST(edit.GetText() == "presetB"); // correct name for a non-first row
    table.SetSelection(std::nullopt);
    BOOST_TEST(edit.GetText() == "");
    // User now types a new name after having selected a preset
    table.SetSelection(0u);
    edit.SetText("presetC");
    base.Msg_EditEnter(0);

    BOOST_TEST(numPresets() == 3u);
    BOOST_TEST(load("presetC") == statesNew); // saved under the typed name
    BOOST_TEST(load("presetA") == statesA);   // selected preset untouched
}

BOOST_FIXTURE_TEST_CASE(AddonPresetDelete, AddonPresetFixture)
{
    save({{1, 2}}, "toDelete");
    BOOST_TEST_REQUIRE(numPresets() == 1u);

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    BOOST_TEST_REQUIRE(select(base, "toDelete"));
    base.Msg_MsgBoxResult(iwAddonPresetsBase::ID_mbDelete, MsgboxResult::Yes);

    BOOST_TEST(numPresets() == 0u); // file removed
    // Deleting drops the selection, so both actions are unavailable again
    BOOST_TEST(!wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction)->GetEnabled());
    BOOST_TEST(!wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete)->GetEnabled());
}

BOOST_FIXTURE_TEST_CASE(AddonPresetDeleteConfirmationNamesPreset, AddonPresetFixture)
{
    save({{1, 2}}, "toDelete");

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    BOOST_TEST_REQUIRE(select(base, "toDelete"));
    base.Msg_ButtonClick(iwAddonPresetsBase::ID_btDelete);

    const auto* msgbox = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(msgbox);
    BOOST_TEST(msgbox->GetTitle() == _("Delete Preset"));
    bool namesPreset = false;
    for(const auto* ml : msgbox->GetCtrls<ctrlMultiline>())
    {
        for(unsigned i = 0; i < ml->GetNumLines(); ++i)
            namesPreset |= ml->GetLine(i).find("toDelete") != std::string::npos;
    }
    BOOST_TEST(namesPreset);
    WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(msgbox));
}

// Loading and deleting act on the list selection, so both stay unavailable until one is picked.
BOOST_FIXTURE_TEST_CASE(AddonPresetActionsRequireSelection, AddonPresetFixture)
{
    save({{1, 2}}, "exists");

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    BOOST_TEST(wnd.GetCtrls<ctrlEdit>().empty()); // no name field, the list is the only target
    BOOST_TEST(!wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction)->GetEnabled());
    BOOST_TEST(!wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete)->GetEnabled());

    BOOST_TEST(!select(base, "notSaved"));
    BOOST_TEST_REQUIRE(select(base, "exists"));
    BOOST_TEST(wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction)->GetEnabled());
    BOOST_TEST(wnd.GetCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete)->GetEnabled());
}

// When the presets folder can't be created, the user is told and no preset window is opened.
BOOST_FIXTURE_TEST_CASE(AddonPresetFolderUnavailable, AddonPresetTmpUserData)
{
    // Plant a file where the presets folder should be so create_directories() fails
    const auto presetsDir = RTTRCONFIG.ExpandPath(s25::folders::addonPresets);
    {
        std::ofstream blocker(presetsDir.string());
        blocker << 'x';
    }
    BOOST_TEST_REQUIRE(boost::filesystem::exists(presetsDir));
    BOOST_TEST_REQUIRE(!boost::filesystem::is_directory(presetsDir));

    BOOST_TEST(!iwAddonPresetsBase::EnsurePresetsFolder());

    const auto* msgbox = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(msgbox);
    BOOST_TEST(msgbox->GetTitle() == _("Addon Presets Unavailable"));
    WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(msgbox));
}

namespace {
MOCK_BASE_CLASS(TestWindow, Window)
{
public:
    TestWindow(Window * parent, unsigned id, const DrawPoint& position) : Window(parent, id, position) {}
    MOCK_METHOD(Msg_PaintBefore, 0)
    MOCK_METHOD(Msg_PaintAfter, 0)
    MOCK_METHOD(Draw_, 0, void())
};
} // namespace

BOOST_AUTO_TEST_CASE(DrawOrder)
{
    Desktop* dsk = WINDOWMANAGER.GetCurrentDesktop();
    std::vector<TestWindow*> wnds;
    wnds.reserve(6);
    // Top level controls
    for(int i = 0; i < 3; i++)
    {
        wnds.push_back(
          dsk->AddCtrl(std::make_unique<TestWindow>(dsk, static_cast<unsigned>(wnds.size()), DrawPoint(0, 0))));
    }
    // Some groups with own controls
    for(int i = 0; i < 3; i++)
    {
        ctrlGroup* grp = dsk->AddGroup(100 + i);
        for(int i = 0; i < 3; i++)
        {
            wnds.push_back(
              grp->AddCtrl(std::make_unique<TestWindow>(dsk, static_cast<unsigned>(wnds.size()), DrawPoint(0, 0))));
        }
    }
    mock::sequence s;
    // Note: Actually order of calls to controls is undefined but in practice matches the IDs
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Msg_PaintBefore).once().in(s);
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Draw_).once().in(s);
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Msg_PaintAfter).once().in(s);
    WINDOWMANAGER.Draw();
    mock::verify();
}

BOOST_AUTO_TEST_SUITE_END()
