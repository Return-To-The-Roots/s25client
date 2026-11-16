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
#include "rttr/test/testHelpers.hpp"
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
    BOOST_TEST_REQUIRE(wnd.getCtrls<ctrlButton>().size() == 2u);
    // Find a text field containing all winner names
    const auto txts = wnd.getCtrls<ctrlMultiline>();
    bool found = false;
    for(const ctrlMultiline* txt : txts)
    {
        if(txt->getNumLines() != winnerNames.size())
            continue; // LCOV_EXCL_LINE
        bool curFound = true;
        for(unsigned i = 0; i < winnerNames.size(); i++)
        {
            curFound &= txt->getLine(i) == winnerNames[i];
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
    const auto addonsChangeableGui = wndAllChangeable.getCtrls<ctrlGroup>();
    BOOST_TEST_REQUIRE(addonsChangeableGui.size() == ggs.getNumAddons() + 1); // First element is the option group
    const auto addonsReadonlyGui = wndAllReadOnly.getCtrls<ctrlGroup>();
    BOOST_TEST_REQUIRE(addonsReadonlyGui.size() == ggs.getNumAddons() + 1);
    for(unsigned i = 1; i <= ggs.getNumAddons(); ++i)
    {
        const ctrlGroup* changeableGroup = addonsChangeableGui[i];
        const ctrlGroup* readonlyGroup = addonsReadonlyGui[i];
        // No lock icon
        BOOST_TEST(changeableGroup->getCtrls<ctrlImage>().empty());
        // Lock icon
        BOOST_TEST_REQUIRE(!readonlyGroup->getCtrls<ctrlImage>().empty());
        // Verify it is the lock icon with tooltip
        BOOST_TEST_REQUIRE(readonlyGroup->getCtrls<ctrlImage>()[0]->getTooltip() == _("Locked"));
        for(const auto* checkbox : changeableGroup->getCtrls<ctrlCheck>())
            BOOST_TEST_REQUIRE(!checkbox->isReadOnly());
        for(const auto* checkbox : readonlyGroup->getCtrls<ctrlCheck>())
            BOOST_TEST_REQUIRE(checkbox->isReadOnly());
        for(const auto* cb : changeableGroup->getCtrls<ctrlComboBox>())
            BOOST_TEST_REQUIRE(!cb->isReadOnly());
        for(const auto* cb : readonlyGroup->getCtrls<ctrlComboBox>())
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
    const auto bts = wnd.getCtrls<ctrlTextButton>();
    BOOST_TEST(bts.size() > 4);
    const auto numIncBts = helpers::count_if(bts, [](const ctrlTextButton* bt) { return bt->getText().at(0) == '+'; });
    BOOST_TEST(numIncBts >= 4);
}

namespace {
struct AddonPresetFixture : uiHelper::Fixture
{
    rttr::test::TmpFolder tmp;
    rttr::test::ConfigOverride userDataOverride{"USERDATA", tmp};

    // Stand in for the game start, which creates all user folders
    AddonPresetFixture() { boost::filesystem::create_directories(RTTRCONFIG.ExpandPath(s25::folders::addonPresets)); }

    // Selects the named preset, false if there is no such preset
    static bool select(Window& wnd, const std::string& name)
    {
        auto& table = *wnd.getCtrls<ctrlTable>().at(0);
        for(unsigned short i = 0; i < table.getNumRows(); ++i)
        {
            if(table.getItemText(i, 0) == name)
            {
                table.setSelection(i);
                return true;
            }
        }
        return false;
    }

    static iwMsgbox* topMsgbox() { return dynamic_cast<iwMsgbox*>(WINDOWMANAGER.getTopMostWindow()); }

    static bool mentions(const iwMsgbox& msgbox, const std::string& name)
    {
        bool found = false;
        for(const auto* ml : msgbox.getCtrls<ctrlMultiline>())
        {
            for(unsigned i = 0; i < ml->getNumLines(); ++i)
                found |= ml->getLine(i).find(name) != std::string::npos;
        }
        return found;
    }

    static void closeMsgbox(iwMsgbox& msgbox) { WINDOWMANAGER.CloseNow(&msgbox); }

    // Answers the prompt on top and closes it, as its own button would.
    // Anything the answer opens (e.g. an error) stays open.
    static void answerMsgbox(Window& wnd, unsigned msgboxId, MsgboxResult result)
    {
        auto& prompt = ensureNonNull(topMsgbox());
        wnd.msgMsgBoxResult(msgboxId, result);
        closeMsgbox(prompt);
    }

    void save(const std::map<unsigned, unsigned>& states, const std::string& name)
    {
        iwSaveAddonPreset wnd(states);
        Window& base = wnd;
        base.getCtrls<ctrlEdit>().at(0)->setText(name);
        base.msgButtonClick(iwAddonPresetsBase::ID_btAction);
    }

    // Returns the given preset's settings, or empty if the preset is missing or corrupt.
    std::map<unsigned, unsigned> load(const std::string& name)
    {
        std::map<unsigned, unsigned> out;
        iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>& s) { out = s; });
        Window& base = wnd;
        if(select(base, name))
            base.msgButtonClick(iwAddonPresetsBase::ID_btAction);
        return out;
    }

    // Presets currently on disk, read via a fresh Load window.
    unsigned numPresets()
    {
        iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
        return wnd.getCtrls<ctrlTable>().at(0)->getNumRows();
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(AddonPresetSaveLoadRoundtrip, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> states{{1, 2}, {3, 0}};
    save(states, "myPreset");
    BOOST_TEST(load("myPreset") == states);
}

BOOST_FIXTURE_TEST_CASE(AddonPresetOverwrite, AddonPresetFixture)
{
    const std::map<unsigned, unsigned> oldStates{{1, 2}, {3, 0}};
    const std::map<unsigned, unsigned> newStates{{3, 4}};
    save(oldStates, "myPreset");

    // Answering No
    {
        iwSaveAddonPreset wnd(newStates);
        Window& base = wnd;
        base.getCtrls<ctrlEdit>().at(0)->setText("myPreset");
        base.msgEditEnter(0);

        const auto& msgbox = ensureNonNull(topMsgbox());
        BOOST_TEST(msgbox.getTitle() == _("Overwrite Preset"));
        BOOST_TEST(mentions(msgbox, "myPreset"));

        answerMsgbox(base, iwSaveAddonPreset::ID_mbOverwrite, MsgboxResult::No);
    }
    BOOST_TEST(load("myPreset") == oldStates);

    // Answering Yes
    {
        iwSaveAddonPreset wnd(newStates);
        Window& base = wnd;
        base.getCtrls<ctrlEdit>().at(0)->setText("myPreset");
        base.msgEditEnter(0);
        answerMsgbox(base, iwSaveAddonPreset::ID_mbOverwrite, MsgboxResult::Yes);
    }
    BOOST_TEST(load("myPreset") == newStates);
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
    base.msgTableChooseItem(iwAddonPresetsBase::ID_tblPresets, 0u);

    BOOST_TEST_REQUIRE(loaded.has_value());
    BOOST_TEST(*loaded == states);
}

BOOST_FIXTURE_TEST_CASE(AddonPresetLoadCorrupt, AddonPresetFixture)
{
    save({{1, 2}}, "valid");
    {
        std::ofstream corrupt((RTTRCONFIG.ExpandPath(s25::folders::addonPresets) / "corrupt.ini").string());
        corrupt << "[addons]\nnotAnAddonId=1\n";
    }

    unsigned numLoaded = 0;
    iwLoadAddonPreset wnd([&](const std::map<unsigned, unsigned>&) noexcept { ++numLoaded; });
    Window& base = wnd;
    BOOST_TEST_REQUIRE(select(base, "valid"));
    base.msgButtonClick(iwAddonPresetsBase::ID_btAction);
    BOOST_TEST_REQUIRE(numLoaded == 1u);

    BOOST_TEST_REQUIRE(select(base, "corrupt"));
    base.msgButtonClick(iwAddonPresetsBase::ID_btAction);

    BOOST_TEST(numLoaded == 1u); // nothing is applied from a corrupt preset
    auto& msgbox = ensureNonNull(topMsgbox());
    BOOST_TEST(msgbox.getTitle() == _("Load Failed"));
    closeMsgbox(msgbox);
}

// A failed save reports the problem but keeps the window and the name so it can be retried
BOOST_FIXTURE_TEST_CASE(AddonPresetSaveUnwritable, AddonPresetFixture)
{
    // A folder in the file's place stands in for a presets folder we may not write to
    boost::filesystem::create_directories(RTTRCONFIG.ExpandPath(s25::folders::addonPresets) / "blocked.ini");

    const std::map<unsigned, unsigned> states{{1, 2}};
    iwSaveAddonPreset wnd(states);
    Window& base = wnd;
    base.getCtrls<ctrlEdit>().at(0)->setText("blocked");
    base.msgButtonClick(iwAddonPresetsBase::ID_btAction);
    answerMsgbox(base, iwSaveAddonPreset::ID_mbOverwrite, MsgboxResult::Yes);

    auto& msgbox = ensureNonNull(topMsgbox());
    BOOST_TEST(msgbox.getTitle() == _("Save Failed"));
    BOOST_TEST(!wnd.ShouldBeClosed());
    BOOST_TEST(base.getCtrls<ctrlEdit>().at(0)->getText() == "blocked");
    closeMsgbox(msgbox);
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
    const auto& deleteBtn = ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete));
    BOOST_TEST(!deleteBtn.getEnabled());
    auto& edit = *wnd.getCtrls<ctrlEdit>().at(0);
    auto& table = *wnd.getCtrls<ctrlTable>().at(0);
    // Rows are sorted ascending, so row 0 is presetA
    table.setSelection(0u);
    BOOST_TEST_REQUIRE(edit.getText() == "presetA");
    BOOST_TEST(deleteBtn.getEnabled());
    table.setSelection(1u);
    BOOST_TEST(edit.getText() == "presetB"); // correct name for a non-first row
    table.setSelection(std::nullopt);
    BOOST_TEST(edit.getText() == "presetB"); // only picking a preset replaces the name
    BOOST_TEST(!deleteBtn.getEnabled());
    // User now types a new name after having selected a preset
    table.setSelection(0u);
    BOOST_TEST_REQUIRE(deleteBtn.getEnabled());
    edit.setText("presetC");
    // Editing drops the selection, so there is nothing left to delete
    BOOST_TEST(!table.getSelection().has_value());
    BOOST_TEST(!deleteBtn.getEnabled());
    BOOST_TEST(edit.getText() == "presetC");
    base.msgEditEnter(0);

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
    base.msgButtonClick(iwAddonPresetsBase::ID_btDelete);

    const auto& msgbox = ensureNonNull(topMsgbox());
    BOOST_TEST(msgbox.getTitle() == _("Delete Preset"));
    BOOST_TEST(mentions(msgbox, "toDelete"));

    answerMsgbox(base, iwAddonPresetsBase::ID_mbDelete, MsgboxResult::Yes);

    BOOST_TEST(numPresets() == 0u);
    // Deleting drops the selection, so both actions are unavailable again
    BOOST_TEST(!ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction)).getEnabled());
    BOOST_TEST(!ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete)).getEnabled());
}

BOOST_FIXTURE_TEST_CASE(AddonPresetSaveWindowCanDelete, AddonPresetFixture)
{
    save({{1, 2}}, "toDelete");

    iwSaveAddonPreset wnd(std::map<unsigned, unsigned>{{9, 9}});
    Window& base = wnd;
    BOOST_TEST_REQUIRE(select(base, "toDelete"));
    base.msgButtonClick(iwAddonPresetsBase::ID_btDelete);
    answerMsgbox(base, iwAddonPresetsBase::ID_mbDelete, MsgboxResult::Yes);

    BOOST_TEST(numPresets() == 0u);
    BOOST_TEST(!ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete)).getEnabled());
    // The name stays, so the preset can be saved again right away
    BOOST_TEST(base.getCtrls<ctrlEdit>().at(0)->getText() == "toDelete");
    BOOST_TEST(ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction)).getEnabled());
}

// Loading and deleting act on the list selection, so both stay unavailable until one is picked.
BOOST_FIXTURE_TEST_CASE(AddonPresetActionsRequireSelection, AddonPresetFixture)
{
    save({{1, 2}}, "exists");

    iwLoadAddonPreset wnd([](const std::map<unsigned, unsigned>&) noexcept {});
    Window& base = wnd;
    const auto& loadBtn = ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction));
    const auto& deleteBtn = ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btDelete));
    BOOST_TEST(wnd.getCtrls<ctrlEdit>().empty()); // no name field, the list is the only target
    BOOST_TEST(!loadBtn.getEnabled());
    BOOST_TEST(!deleteBtn.getEnabled());

    BOOST_TEST(!select(base, "notSaved"));
    BOOST_TEST_REQUIRE(select(base, "exists"));
    BOOST_TEST(loadBtn.getEnabled());
    BOOST_TEST(deleteBtn.getEnabled());
}

// Saving needs a name, typed or taken from a picked preset
BOOST_FIXTURE_TEST_CASE(AddonPresetSaveRequiresName, AddonPresetFixture)
{
    save({{1, 2}}, "exists");

    iwSaveAddonPreset wnd(std::map<unsigned, unsigned>{{3, 4}});
    Window& base = wnd;
    const auto& saveBtn = ensureNonNull(wnd.getCtrl<ctrlButton>(iwAddonPresetsBase::ID_btAction));
    auto& edit = *wnd.getCtrls<ctrlEdit>().at(0);
    BOOST_TEST(!saveBtn.getEnabled());

    edit.setText("   ");
    BOOST_TEST(!saveBtn.getEnabled()); // spaces alone are no name
    edit.setText("new");
    BOOST_TEST(saveBtn.getEnabled());
    edit.setText("nul");
    BOOST_TEST(saveBtn.getEnabled()); // an invalid name is reported when saving instead
    edit.setText("");
    BOOST_TEST(!saveBtn.getEnabled());

    BOOST_TEST_REQUIRE(select(base, "exists"));
    BOOST_TEST(saveBtn.getEnabled());
}

namespace {
MOCK_BASE_CLASS(TestWindow, Window)
{
public:
    TestWindow(Window * parent, unsigned id, const DrawPoint& position) : Window(parent, id, position) {}
    MOCK_METHOD(msgPaintBefore, 0)
    MOCK_METHOD(msgPaintAfter, 0)
    MOCK_METHOD(Draw_, 0, void())
};
} // namespace

BOOST_AUTO_TEST_CASE(DrawOrder)
{
    Desktop* dsk = WINDOWMANAGER.getCurrentDesktop();
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
        MOCK_EXPECT(wnd->msgPaintBefore).once().in(s);
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Draw_).once().in(s);
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->msgPaintAfter).once().in(s);
    WINDOWMANAGER.Draw();
    mock::verify();
}

BOOST_AUTO_TEST_SUITE_END()
