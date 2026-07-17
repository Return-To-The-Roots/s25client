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
struct AddonPresetFixture : uiHelper::Fixture
{
    rttr::test::TmpFolder tmp;
    rttr::test::ConfigOverride userDataOverride{"USERDATA", tmp};

    void save(const std::map<unsigned, unsigned>& states, const std::string& name)
    {
        iwSaveAddonPreset wnd(states);
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText(name);
        base.Msg_EditEnter(0);
    }

    // Returns the states passed to the callback, or empty if it was not invoked (name not found).
    std::map<unsigned, unsigned> load(const std::string& name)
    {
        std::map<unsigned, unsigned> out;
        iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>& s) { out = s; });
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText(name);
        base.Msg_EditEnter(0);
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

// A name already ending in the extension is a distinct preset, independently loadable and deletable.
BOOST_FIXTURE_TEST_CASE(AddonPresetExtensionInNameIsDistinct, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> states{{1, 2}};
    const std::map<unsigned, unsigned> statesDoubled{{3, 4}};
    save(states, "myPreset");            // -> myPreset.ini,     listed "myPreset"
    save(statesDoubled, "myPreset.ini"); // -> myPreset.ini.ini, listed "myPreset.ini"
    BOOST_TEST_REQUIRE(numPresets() == 2u);

    BOOST_TEST(load("myPreset") == states);
    BOOST_TEST(load("myPreset.ini") == statesDoubled);

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    base.GetCtrls<ctrlEdit>().at(0)->SetText("myPreset.ini");
    base.Msg_MsgBoxResult(iwAddonPresetsBase::ID_mbDelete, MsgboxResult::Yes);
    BOOST_TEST(numPresets() == 1u);
    BOOST_TEST(load("myPreset.ini").empty()); // doubled file gone
    BOOST_TEST(load("myPreset") == states);   // sibling preset untouched
}

// The edit box is the source of truth: after selecting a preset, editing the name and acting
// must target the edited name, not the stale table selection.
BOOST_FIXTURE_TEST_CASE(AddonPresetEditOverridesSelection, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> statesA{{1, 2}};
    const std::map<unsigned, unsigned> statesB{{3, 4}};
    save(statesA, "presetA");
    save(statesB, "presetB");

    std::optional<std::map<unsigned, unsigned>> loaded;
    iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>& s) { loaded = s; });
    Window& base = wnd;
    auto& edit = *wnd.GetCtrls<ctrlEdit>().at(0);
    auto& table = *wnd.GetCtrls<ctrlTable>().at(0);
    // Selection drives the edit: each selected row's name lands in the edit (rows sorted ascending)
    table.SetSelection(0u);
    BOOST_TEST_REQUIRE(edit.GetText() == "presetA");
    table.SetSelection(1u);
    BOOST_TEST(edit.GetText() == "presetB"); // correct name for a non-first row
    table.SetSelection(std::nullopt);        // deselect
    BOOST_TEST(edit.GetText() == "");
    table.SetSelection(0u);
    BOOST_TEST_REQUIRE(edit.GetText() == "presetA");
    // User now retypes a different existing preset
    edit.SetText("presetB");
    base.Msg_EditEnter(0);
    BOOST_TEST_REQUIRE(loaded.has_value());
    BOOST_TEST(*loaded == statesB);
}

BOOST_FIXTURE_TEST_CASE(AddonPresetDelete, AddonPresetFixture)
{
    save({{1, 2}}, "toDelete");
    BOOST_TEST_REQUIRE(numPresets() == 1u);

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    base.GetCtrls<ctrlEdit>().at(0)->SetText("toDelete");
    base.Msg_MsgBoxResult(iwAddonPresetsBase::ID_mbDelete, MsgboxResult::Yes);

    BOOST_TEST(base.GetCtrls<ctrlEdit>().at(0)->GetText() == ""); // edit cleared after delete
    BOOST_TEST(numPresets() == 0u);                               // file removed
}

BOOST_FIXTURE_TEST_CASE(AddonPresetDeleteConfirmationNamesPreset, AddonPresetFixture)
{
    save({{1, 2}}, "toDelete");

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    base.GetCtrls<ctrlEdit>().at(0)->SetText("toDelete");
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

// Loading/deleting a name that doesn't exist informs the user and changes nothing.
BOOST_FIXTURE_TEST_CASE(AddonPresetTargetNotFound, AddonPresetFixture)
{
    save({{1, 2}}, "exists");

    // Load a missing name -> callback not invoked, "Preset Not Found" shown
    {
        bool called = false;
        iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>&) noexcept { called = true; });
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText("missing");
        base.Msg_EditEnter(0);
        BOOST_TEST(!called);
        const auto* msgbox = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
        BOOST_TEST_REQUIRE(msgbox);
        BOOST_TEST(msgbox->GetTitle() == _("Preset Not Found"));
        WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(msgbox));
    }

    // Delete a missing name -> "Preset Not Found" shown (not the delete confirmation)
    {
        iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
        Window& base = wnd;
        base.GetCtrls<ctrlEdit>().at(0)->SetText("missing");
        base.Msg_ButtonClick(iwAddonPresetsBase::ID_btDelete);
        const auto* msgbox = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
        BOOST_TEST_REQUIRE(msgbox);
        BOOST_TEST(msgbox->GetTitle() == _("Preset Not Found"));
        WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(msgbox));
    }

    BOOST_TEST(numPresets() == 1u); // "exists" untouched
}

BOOST_FIXTURE_TEST_CASE(AddonPresetEmptyNameNoOp, AddonPresetFixture)
{
    save({{1, 2}}, "exists");

    // Load with empty edit -> callback not invoked, no message
    {
        bool called = false;
        iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>&) noexcept { called = true; });
        Window& base = wnd;
        base.Msg_EditEnter(0);
        BOOST_TEST(!called);
        BOOST_TEST(!dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow()));
    }
    // Delete with empty edit -> no message
    {
        iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
        Window& base = wnd;
        base.Msg_ButtonClick(iwAddonPresetsBase::ID_btDelete);
        BOOST_TEST(!dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow()));
    }

    BOOST_TEST(numPresets() == 1u); // nothing deleted
}

// When the presets folder can't be created, the window informs the user and closes itself.
BOOST_FIXTURE_TEST_CASE(AddonPresetFolderUnavailable, AddonPresetFixture)
{
    // Plant a file where the presets folder should be so create_directories() fails
    const auto presetsDir = RTTRCONFIG.ExpandPath(s25::folders::addonPresets);
    {
        std::ofstream blocker(presetsDir.string());
        blocker << 'x';
    }
    BOOST_TEST_REQUIRE(boost::filesystem::exists(presetsDir));
    BOOST_TEST_REQUIRE(!boost::filesystem::is_directory(presetsDir));

    iwSaveAddonPreset wnd(std::map<unsigned, unsigned>{{1, 2}});
    BOOST_TEST(wnd.ShouldBeClosed());              // window marked itself for closing
    BOOST_TEST(wnd.GetCtrls<ctrlTable>().empty()); // no controls were built

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
