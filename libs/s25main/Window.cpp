// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Window.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "RescaleWindowProp.h"
#include "commonDefines.h"
#include "controls/controls.h"
#include "driver/MouseCoords.h"
#include "drivers/ScreenResizeEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "ogl/IRenderer.h"
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <cstdarg>

Window::Window(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size)
    : parent_(parent), id_(id), pos_(pos), size_(size), active_(false), visible_(true), scale_(false),
      isInMouseRelay(false), animations_(this)
{}

Window::~Window()
{
    RTTR_Assert(!isInMouseRelay);
}

void Window::Draw()
{
    if(visible_)
        Draw_();
}

DrawPoint Window::getPos() const
{
    return pos_;
}

DrawPoint Window::getDrawPos() const
{
    DrawPoint result = pos_;
    const Window* temp = this;

    // Convert relative to absolute coordinates, i.e. sum positions of parents
    while(temp->parent_)
    {
        temp = temp->parent_;
        result += temp->pos_;
    }

    return result;
}

Extent Window::getSize() const
{
    return size_;
}

Rect Window::getDrawRect() const
{
    return Rect(getDrawPos(), getSize());
}

Rect Window::getBoundaryRect() const
{
    // Default to draw rect
    return getDrawRect();
}

bool Window::RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke)
{
    // Ask derived classes whether relaying messages is allowed
    // (For example, ingame windows might not want to receive keyboard messages when they are minimized)
    if(!isMessageRelayAllowed())
        return false;

    for(auto& wnd : childIdToWnd_ | boost::adaptors::map_values)
    {
        if(wnd->visible_ && wnd->active_ && CALL_MEMBER_FN(*wnd, msg)(ke))
            return true;
    }

    return false;
}

bool Window::RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc)
{
    // Ask derived classes whether relaying messages is allowed
    if(!isMessageRelayAllowed())
        return false;

    bool processed = false;
    isInMouseRelay = true;

    // Use reverse iterator because the topmost (=last elements) should receive the messages first!
    for(auto& wnd : childIdToWnd_ | boost::adaptors::map_values | boost::adaptors::reversed)
    {
        if(!lockedAreas_.empty() && isInLockedRegion(mc.pos, wnd.get()))
            continue;

        if(wnd->visible_ && wnd->active_ && CALL_MEMBER_FN(*wnd, msg)(mc))
            processed = true;
    }

    for(auto* tofreeArea : tofreeAreas_)
        lockedAreas_.erase(tofreeArea);
    tofreeAreas_.clear();
    isInMouseRelay = false;

    return processed;
}

/**
 *  aktiviert das Fenster.
 *
 *  @param[in] activate Fenster aktivieren?
 */
void Window::setActive(bool activate)
{
    active_ = activate;
    ActivateControls(activate);
}

/**
 *  aktiviert die Steuerelemente des Fensters.
 *
 *  @param[in] activate Steuerelemente aktivieren?
 */
void Window::ActivateControls(bool activate)
{
    for(auto& it : childIdToWnd_)
        it.second->setActive(activate);
}

/**
 *  sperrt eine Region eines Fensters.
 *
 *  @param[in] window das Fenster, welches die Region sperrt.
 *  @param[in] rect   das Rechteck, welches die Region beschreibt.
 */
void Window::LockRegion(Window* window, const Rect& rect)
{
    lockedAreas_[window] = rect;
    auto it = helpers::find(tofreeAreas_, window);
    if(it != tofreeAreas_.end())
        tofreeAreas_.erase(it);

    // Also lock the region for all parents
    if(getParent())
        getParent()->LockRegion(this, rect);
}

/**
 *  Gibt eine gesperrte Region wieder frei.
 *
 *  @param[in] window das Fenster, welches die Region sperrt.
 */
void Window::FreeRegion(Window* window)
{
    // We need to keep all locked areas otherwise a closed dropdown will enable "click-through" to below control
    if(isInMouseRelay)
        tofreeAreas_.push_back(window);
    else
        lockedAreas_.erase(window);

    // Also free the locked region for all parents
    if(getParent())
        getParent()->FreeRegion(this);
}

void Window::setPos(const DrawPoint& newPos)
{
    pos_ = newPos;
}

bool Window::isMessageRelayAllowed() const
{
    return true;
}

void Window::DeleteCtrl(unsigned id)
{
    childIdToWnd_.erase(id);
}

ctrlBuildingIcon* Window::AddBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, const Nation nation,
                                          unsigned short size, const std::string& tooltip)
{
    return AddCtrl(
      std::make_unique<ctrlBuildingIcon>(this, id, ScaleIf(pos), type, nation, ScaleIf(Extent(size, 0)).x, tooltip));
}

ctrlButton* Window::AddTextButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                  const std::string& text, const glFont* font, const std::string& tooltip)
{
    return AddCtrl(std::make_unique<ctrlTextButton>(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, tooltip));
}

ctrlButton* Window::AddColorButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   const unsigned fillColor, const std::string& tooltip)
{
    return AddCtrl(std::make_unique<ctrlColorButton>(this, id, ScaleIf(pos), ScaleIf(size), tc, fillColor, tooltip));
}

ctrlButton* Window::AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   ITexture* const image, const std::string& tooltip)
{
    return AddCtrl(std::make_unique<ctrlImageButton>(this, id, ScaleIf(pos), ScaleIf(size), tc, image, tooltip));
}

ctrlButton* Window::AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   glArchivItem_Bitmap* const image, const std::string& tooltip)
{
    return AddImageButton(id, pos, size, tc, static_cast<ITexture*>(image), tooltip);
}

ctrlChat* Window::AddChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const glFont* font)
{
    return AddCtrl(std::make_unique<ctrlChat>(this, id, ScaleIf(pos), ScaleIf(size), tc, font));
}

ctrlCheck* Window::AddCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               const std::string& text, const glFont* font, bool readonly)
{
    return AddCtrl(std::make_unique<ctrlCheck>(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, readonly));
}

ctrlComboBox* Window::AddComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                  const glFont* font, unsigned short max_list_height, bool readonly)
{
    return AddCtrl(
      std::make_unique<ctrlComboBox>(this, id, ScaleIf(pos), ScaleIf(size), tc, font, max_list_height, readonly));
}

ctrlDeepening* Window::AddTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                        const std::string& text, const glFont* font, unsigned color, FontStyle style)
{
    return AddCtrl(
      std::make_unique<ctrlTextDeepening>(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, color, style));
}

ctrlDeepening* Window::AddColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         unsigned fillColor)
{
    return AddCtrl(std::make_unique<ctrlColorDeepening>(this, id, ScaleIf(pos), ScaleIf(size), tc, fillColor));
}

ctrlDeepening* Window::AddImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         ITexture* image)
{
    return AddCtrl(std::make_unique<ctrlImageDeepening>(this, id, ScaleIf(pos), ScaleIf(size), tc, image));
}

ctrlDeepening* Window::AddImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         glArchivItem_Bitmap* image)
{
    return AddImageDeepening(id, pos, size, tc, static_cast<ITexture*>(image));
}

ctrlEdit* Window::AddEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                          unsigned short maxlength, bool password, bool disabled, bool notify)
{
    return AddCtrl(std::make_unique<ctrlEdit>(this, id, ScaleIf(pos), ScaleIf(size), tc, font, maxlength, password,
                                              disabled, notify));
}

ctrlGroup* Window::AddGroup(unsigned id)
{
    return AddCtrl(std::make_unique<ctrlGroup>(this, id));
}

ctrlImage* Window::AddImage(unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip)
{
    return AddCtrl(std::make_unique<ctrlImage>(this, id, ScaleIf(pos), image, tooltip));
}

ctrlImage* Window::AddImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip)
{
    return AddImage(id, pos, static_cast<ITexture*>(image), tooltip);
}

ctrlList* Window::AddList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font)
{
    return AddCtrl(std::make_unique<ctrlList>(this, id, ScaleIf(pos), ScaleIf(size), tc, font));
}

ctrlMultiline* Window::AddMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                    const glFont* font, FontStyle format)
{
    return AddCtrl(std::make_unique<ctrlMultiline>(this, id, ScaleIf(pos), ScaleIf(size), tc, font, format));
}

ctrlOptionGroup* Window::AddOptionGroup(unsigned id, GroupSelectType select_type)
{
    return AddCtrl(std::make_unique<ctrlOptionGroup>(this, id, select_type));
}

ctrlMultiSelectGroup* Window::AddMultiSelectGroup(unsigned id, GroupSelectType select_type)
{
    return AddCtrl(std::make_unique<ctrlMultiSelectGroup>(this, id, select_type));
}

ctrlPercent* Window::AddPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                unsigned text_color, const glFont* font, const unsigned short* percentage)
{
    return AddCtrl(
      std::make_unique<ctrlPercent>(this, id, ScaleIf(pos), ScaleIf(size), tc, text_color, font, percentage));
}

ctrlProgress* Window::AddProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                  unsigned short button_minus, unsigned short button_plus, unsigned short maximum,
                                  const std::string& tooltip, const Extent& padding, unsigned force_color,
                                  const std::string& button_minus_tooltip, const std::string& button_plus_tooltip)
{
    return AddCtrl(std::make_unique<ctrlProgress>(this, id, ScaleIf(pos), ScaleIf(size), tc, button_minus, button_plus,
                                                  maximum, padding, force_color, tooltip, button_minus_tooltip,
                                                  button_plus_tooltip));
}

ctrlScrollBar* Window::AddScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height,
                                    TextureColor tc, unsigned short page_size)
{
    button_height = ScaleIf(Extent(0, button_height)).y;

    return AddCtrl(
      std::make_unique<ctrlScrollBar>(this, id, ScaleIf(pos), ScaleIf(size), button_height, tc, page_size));
}

ctrlTab* Window::AddTabCtrl(unsigned id, const DrawPoint& pos, unsigned short width)
{
    return AddCtrl(std::make_unique<ctrlTab>(this, id, ScaleIf(pos), ScaleIf(Extent(width, 0)).x));
}

ctrlTable* Window::AddTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                            std::vector<TableColumn> columns)
{
    return AddCtrl(std::make_unique<ctrlTable>(this, id, ScaleIf(pos), ScaleIf(size), tc, font, std::move(columns)));
}

ctrlTimer* Window::AddTimer(unsigned id, std::chrono::milliseconds timeout)
{
    return AddCtrl(std::make_unique<ctrlTimer>(this, id, timeout));
}

ctrlText* Window::AddText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                          const glFont* font)
{
    return AddCtrl(std::make_unique<ctrlText>(this, id, ScaleIf(pos), text, color, format, font));
}

ctrlMapSelection* Window::AddMapSelection(unsigned id, const DrawPoint& pos, const Extent& size,
                                          const SelectionMapInputData& inputData)
{
    return AddCtrl(std::make_unique<ctrlMapSelection>(this, id, ScaleIf(pos), ScaleIf(size), inputData));
}

TextFormatSetter Window::AddFormattedText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
                                          FontStyle format, const glFont* font)
{
    return AddText(id, pos, text, color, format, font);
}

ctrlVarDeepening* Window::AddVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                          const std::string& formatstr, const glFont* font, unsigned color,
                                          unsigned parameters, ...)
{
    va_list liste;
    va_start(liste, parameters);

    auto ctrl = std::make_unique<ctrlVarDeepening>(this, id, ScaleIf(pos), ScaleIf(size), tc, formatstr, font, color,
                                                   parameters, liste);

    va_end(liste);

    return AddCtrl(std::move(ctrl));
}

ctrlVarText* Window::AddVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                                FontStyle format, const glFont* font, unsigned parameters, ...)
{
    va_list liste;
    va_start(liste, parameters);

    auto ctrl =
      std::make_unique<ctrlVarText>(this, id, ScaleIf(pos), formatstr, color, format, font, parameters, liste);

    va_end(liste);

    return AddCtrl(std::move(ctrl));
}

ctrlPreviewMinimap* Window::AddPreviewMinimap(const unsigned id, const DrawPoint& pos, const Extent& size,
                                              libsiedler2::ArchivItem_Map* const map)
{
    return AddCtrl(std::make_unique<ctrlPreviewMinimap>(this, id, ScaleIf(pos), ScaleIf(size), map));
}

void Window::Draw3D(const Rect& rect, TextureColor tc, bool elevated, bool highlighted, bool illuminated,
                    unsigned contentColor)
{
    const Extent rectSize = rect.getSize();
    if(rectSize.x < 4 || rectSize.y < 4)
        return;
    Draw3DBorder(rect, tc, elevated);
    // Move content inside border
    Rect contentRect(rect.getOrigin() + Position(2, 2), rectSize - Extent(4, 4));
    Draw3DContent(contentRect, tc, elevated, highlighted, illuminated, contentColor);
}

void Window::Draw3DBorder(const Rect& rect, TextureColor tc, bool elevated)
{
    if(tc == TextureColor::Invisible)
        return;
    glArchivItem_Bitmap* borderImg = LOADER.getImageN("io", 12 + rttr::enum_cast(tc));
    VIDEODRIVER.getRenderer()->Draw3DBorder(rect, elevated, *borderImg);
}

void Window::Draw3DContent(const Rect& rect, TextureColor tc, bool elevated, bool highlighted, bool illuminated,
                           unsigned contentColor)
{
    if(tc == TextureColor::Invisible)
        return;
    glArchivItem_Bitmap* contentImg = LOADER.getImageN("io", rttr::enum_cast(tc) * 2 + (highlighted ? 0 : 1));
    VIDEODRIVER.getRenderer()->Draw3DContent(rect, elevated, *contentImg, illuminated, contentColor);
}

void Window::DrawRectangle(const Rect& rect, unsigned color)
{
    VIDEODRIVER.getRenderer()->DrawRect(rect, color);
}

void Window::DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color)
{
    VIDEODRIVER.getRenderer()->DrawLine(pt1, pt2, width, color);
}

void Window::msgPaintBefore()
{
    animations_.update(VIDEODRIVER.getTickCount());
    for(auto& control : childIdToWnd_ | boost::adaptors::map_values)
        control->msgPaintBefore();
}

void Window::msgPaintAfter()
{
    for(auto& control : childIdToWnd_ | boost::adaptors::map_values)
        control->msgPaintAfter();
}

void Window::Draw_()
{
    for(auto& control : childIdToWnd_ | boost::adaptors::map_values)
        control->Draw();
}

void Window::msgScreenResize(const ScreenResizeEvent& sr)
{
    // If the window elements don't get scaled there is nothing to do
    if(!scale_)
        return;
    RescaleWindowProp rescale(sr.oldSize, sr.newSize);
    for(auto& ctrl : childIdToWnd_ | boost::adaptors::map_values)
    {
        // Save new size (could otherwise be changed(?) in Msg_ScreenResize)
        Extent newSize = rescale(ctrl->getSize());
        ctrl->setPos(rescale(ctrl->getPos()));
        ctrl->msgScreenResize(sr);
        ctrl->Resize(newSize);
    }
    animations_.onRescale(sr);
}

template<class T_Pt>
T_Pt Window::Scale(const T_Pt& pt)
{
    return ScaleWindowPropUp::scale(pt, VIDEODRIVER.getRenderSize());
}

template<class T_Pt>
T_Pt Window::ScaleIf(const T_Pt& pt) const
{
    return scale_ ? Scale(pt) : pt;
}

// Explicit template instantiations for the used types to avoid linker errors
template DrawPoint Window::Scale(const DrawPoint&);
template Extent Window::Scale(const Extent&);
template DrawPoint Window::ScaleIf(const DrawPoint&) const;
template Extent Window::ScaleIf(const Extent&) const;

bool Window::isInLockedRegion(const Position& pos, const Window* exception) const
{
    for(const auto& lockEntry : lockedAreas_)
    {
        // Ignore exception
        if(lockEntry.first == exception)
            continue;
        if(IsPointInRect(pos, lockEntry.second))
            return true;
    }
    return false;
}

bool Window::isMouseOver() const
{
    return isMouseOver(VIDEODRIVER.getMousePos());
}

bool Window::isMouseOver(const MouseCoords& mousePos) const
{
    return IsPointInRect(mousePos.pos, getBoundaryRect());
}
