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

void Window::draw()
{
    if(visible_)
        drawImpl();
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

bool Window::relayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke)
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

bool Window::relayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc)
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
    activateControls(activate);
}

/**
 *  aktiviert die Steuerelemente des Fensters.
 *
 *  @param[in] activate Steuerelemente aktivieren?
 */
void Window::activateControls(bool activate)
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
void Window::lockRegion(Window* window, const Rect& rect)
{
    lockedAreas_[window] = rect;
    auto it = helpers::find(tofreeAreas_, window);
    if(it != tofreeAreas_.end())
        tofreeAreas_.erase(it);

    // Also lock the region for all parents
    if(getParent())
        getParent()->lockRegion(this, rect);
}

/**
 *  Gibt eine gesperrte Region wieder frei.
 *
 *  @param[in] window das Fenster, welches die Region sperrt.
 */
void Window::freeRegion(Window* window)
{
    // We need to keep all locked areas otherwise a closed dropdown will enable "click-through" to below control
    if(isInMouseRelay)
        tofreeAreas_.push_back(window);
    else
        lockedAreas_.erase(window);

    // Also free the locked region for all parents
    if(getParent())
        getParent()->freeRegion(this);
}

void Window::setPos(const DrawPoint& newPos)
{
    pos_ = newPos;
}

bool Window::isMessageRelayAllowed() const
{
    return true;
}

void Window::deleteCtrl(unsigned id)
{
    childIdToWnd_.erase(id);
}

ctrlBuildingIcon* Window::addBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, const Nation nation,
                                          unsigned short size, const std::string& tooltip)
{
    return addCtrl(
      std::make_unique<ctrlBuildingIcon>(this, id, scaleIf(pos), type, nation, scaleIf(Extent(size, 0)).x, tooltip));
}

ctrlButton* Window::addTextButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                  const std::string& text, const glFont* font, const std::string& tooltip)
{
    return addCtrl(std::make_unique<ctrlTextButton>(this, id, scaleIf(pos), scaleIf(size), tc, text, font, tooltip));
}

ctrlButton* Window::addColorButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   const unsigned fillColor, const std::string& tooltip)
{
    return addCtrl(std::make_unique<ctrlColorButton>(this, id, scaleIf(pos), scaleIf(size), tc, fillColor, tooltip));
}

ctrlButton* Window::addImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   ITexture* const image, const std::string& tooltip)
{
    return addCtrl(std::make_unique<ctrlImageButton>(this, id, scaleIf(pos), scaleIf(size), tc, image, tooltip));
}

ctrlButton* Window::addImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   glArchivItem_Bitmap* const image, const std::string& tooltip)
{
    return addImageButton(id, pos, size, tc, static_cast<ITexture*>(image), tooltip);
}

ctrlChat* Window::addChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const glFont* font)
{
    return addCtrl(std::make_unique<ctrlChat>(this, id, scaleIf(pos), scaleIf(size), tc, font));
}

ctrlCheck* Window::addCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               const std::string& text, const glFont* font, bool readonly)
{
    return addCtrl(std::make_unique<ctrlCheck>(this, id, scaleIf(pos), scaleIf(size), tc, text, font, readonly));
}

ctrlComboBox* Window::addComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                  const glFont* font, unsigned short max_list_height, bool readonly)
{
    return addCtrl(
      std::make_unique<ctrlComboBox>(this, id, scaleIf(pos), scaleIf(size), tc, font, max_list_height, readonly));
}

ctrlDeepening* Window::addTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                        const std::string& text, const glFont* font, unsigned color, FontStyle style)
{
    return addCtrl(
      std::make_unique<ctrlTextDeepening>(this, id, scaleIf(pos), scaleIf(size), tc, text, font, color, style));
}

ctrlDeepening* Window::addColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         unsigned fillColor)
{
    return addCtrl(std::make_unique<ctrlColorDeepening>(this, id, scaleIf(pos), scaleIf(size), tc, fillColor));
}

ctrlDeepening* Window::addImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         ITexture* image)
{
    return addCtrl(std::make_unique<ctrlImageDeepening>(this, id, scaleIf(pos), scaleIf(size), tc, image));
}

ctrlDeepening* Window::addImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         glArchivItem_Bitmap* image)
{
    return addImageDeepening(id, pos, size, tc, static_cast<ITexture*>(image));
}

ctrlEdit* Window::addEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                          unsigned short maxlength, bool password, bool disabled, bool notify)
{
    return addCtrl(std::make_unique<ctrlEdit>(this, id, scaleIf(pos), scaleIf(size), tc, font, maxlength, password,
                                              disabled, notify));
}

ctrlGroup* Window::addGroup(unsigned id)
{
    return addCtrl(std::make_unique<ctrlGroup>(this, id));
}

ctrlImage* Window::addImage(unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip)
{
    return addCtrl(std::make_unique<ctrlImage>(this, id, scaleIf(pos), image, tooltip));
}

ctrlImage* Window::addImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip)
{
    return addImage(id, pos, static_cast<ITexture*>(image), tooltip);
}

ctrlList* Window::addList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font)
{
    return addCtrl(std::make_unique<ctrlList>(this, id, scaleIf(pos), scaleIf(size), tc, font));
}

ctrlMultiline* Window::addMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                    const glFont* font, FontStyle format)
{
    return addCtrl(std::make_unique<ctrlMultiline>(this, id, scaleIf(pos), scaleIf(size), tc, font, format));
}

ctrlOptionGroup* Window::addOptionGroup(unsigned id, GroupSelectType select_type)
{
    return addCtrl(std::make_unique<ctrlOptionGroup>(this, id, select_type));
}

ctrlMultiSelectGroup* Window::addMultiSelectGroup(unsigned id, GroupSelectType select_type)
{
    return addCtrl(std::make_unique<ctrlMultiSelectGroup>(this, id, select_type));
}

ctrlPercent* Window::addPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                unsigned text_color, const glFont* font, const unsigned short* percentage)
{
    return addCtrl(
      std::make_unique<ctrlPercent>(this, id, scaleIf(pos), scaleIf(size), tc, text_color, font, percentage));
}

ctrlProgress* Window::addProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                  unsigned short buttonMinus, unsigned short buttonPlus, unsigned short maximum,
                                  const std::string& tooltip, const Extent& padding, unsigned force_color,
                                  const std::string& button_minus_tooltip, const std::string& button_plus_tooltip)
{
    return addCtrl(std::make_unique<ctrlProgress>(this, id, scaleIf(pos), scaleIf(size), tc, buttonMinus, buttonPlus,
                                                  maximum, padding, force_color, tooltip, button_minus_tooltip,
                                                  button_plus_tooltip));
}

ctrlScrollBar* Window::addScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height,
                                    TextureColor tc, unsigned short page_size)
{
    button_height = scaleIf(Extent(0, button_height)).y;

    return addCtrl(
      std::make_unique<ctrlScrollBar>(this, id, scaleIf(pos), scaleIf(size), button_height, tc, page_size));
}

ctrlTab* Window::addTabCtrl(unsigned id, const DrawPoint& pos, unsigned short width)
{
    return addCtrl(std::make_unique<ctrlTab>(this, id, scaleIf(pos), scaleIf(Extent(width, 0)).x));
}

ctrlTable* Window::addTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                            std::vector<TableColumn> columns)
{
    return addCtrl(std::make_unique<ctrlTable>(this, id, scaleIf(pos), scaleIf(size), tc, font, std::move(columns)));
}

ctrlTimer* Window::addTimer(unsigned id, std::chrono::milliseconds timeout)
{
    return addCtrl(std::make_unique<ctrlTimer>(this, id, timeout));
}

ctrlText* Window::addText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                          const glFont* font)
{
    return addCtrl(std::make_unique<ctrlText>(this, id, scaleIf(pos), text, color, format, font));
}

ctrlMapSelection* Window::addMapSelection(unsigned id, const DrawPoint& pos, const Extent& size,
                                          const SelectionMapInputData& inputData)
{
    return addCtrl(std::make_unique<ctrlMapSelection>(this, id, scaleIf(pos), scaleIf(size), inputData));
}

TextFormatSetter Window::addFormattedText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
                                          FontStyle format, const glFont* font)
{
    return addText(id, pos, text, color, format, font);
}

ctrlVarDeepening* Window::addVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                          const std::string& formatstr, const glFont* font, unsigned color,
                                          unsigned parameters, ...)
{
    va_list liste;
    va_start(liste, parameters);

    auto ctrl = std::make_unique<ctrlVarDeepening>(this, id, scaleIf(pos), scaleIf(size), tc, formatstr, font, color,
                                                   parameters, liste);

    va_end(liste);

    return addCtrl(std::move(ctrl));
}

ctrlVarText* Window::addVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                                FontStyle format, const glFont* font, unsigned parameters, ...)
{
    va_list liste;
    va_start(liste, parameters);

    auto ctrl =
      std::make_unique<ctrlVarText>(this, id, scaleIf(pos), formatstr, color, format, font, parameters, liste);

    va_end(liste);

    return addCtrl(std::move(ctrl));
}

ctrlPreviewMinimap* Window::addPreviewMinimap(const unsigned id, const DrawPoint& pos, const Extent& size,
                                              libsiedler2::ArchivItem_Map* const map)
{
    return addCtrl(std::make_unique<ctrlPreviewMinimap>(this, id, scaleIf(pos), scaleIf(size), map));
}

void Window::draw3D(const Rect& rect, TextureColor tc, bool elevated, bool highlighted, bool illuminated,
                    unsigned contentColor)
{
    const Extent rectSize = rect.getSize();
    if(rectSize.x < 4 || rectSize.y < 4)
        return;
    draw3DBorder(rect, tc, elevated);
    // Move content inside border
    Rect contentRect(rect.getOrigin() + Position(2, 2), rectSize - Extent(4, 4));
    draw3DContent(contentRect, tc, elevated, highlighted, illuminated, contentColor);
}

void Window::draw3DBorder(const Rect& rect, TextureColor tc, bool elevated)
{
    if(tc == TextureColor::Invisible)
        return;
    glArchivItem_Bitmap* borderImg = LOADER.getImageN("io", 12 + rttr::enumCast(tc));
    VIDEODRIVER.getRenderer()->draw3DBorder(rect, elevated, *borderImg);
}

void Window::draw3DContent(const Rect& rect, TextureColor tc, bool elevated, bool highlighted, bool illuminated,
                           unsigned contentColor)
{
    if(tc == TextureColor::Invisible)
        return;
    glArchivItem_Bitmap* contentImg = LOADER.getImageN("io", rttr::enumCast(tc) * 2 + (highlighted ? 0 : 1));
    VIDEODRIVER.getRenderer()->draw3DContent(rect, elevated, *contentImg, illuminated, contentColor);
}

void Window::drawRectangle(const Rect& rect, unsigned color)
{
    VIDEODRIVER.getRenderer()->drawRect(rect, color);
}

void Window::drawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color)
{
    VIDEODRIVER.getRenderer()->drawLine(pt1, pt2, width, color);
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

void Window::drawImpl()
{
    for(auto& control : childIdToWnd_ | boost::adaptors::map_values)
        control->draw();
}

void Window::msgScreenResize(const ScreenResizeEvent& sr)
{
    // If the window elements don't get scaled there is nothing to do
    if(!scale_)
        return;
    RescaleWindowProp rescale(sr.oldSize, sr.newSize);
    for(auto& ctrl : childIdToWnd_ | boost::adaptors::map_values)
    {
        // Save new size (could otherwise be changed(?) in msgScreenResize)
        Extent newSize = rescale(ctrl->getSize());
        ctrl->setPos(rescale(ctrl->getPos()));
        ctrl->msgScreenResize(sr);
        ctrl->resize(newSize);
    }
    animations_.onRescale(sr);
}

template<class T_Pt>
T_Pt Window::scale(const T_Pt& pt)
{
    return ScaleWindowPropUp::scale(pt, VIDEODRIVER.getRenderSize());
}

template<class T_Pt>
T_Pt Window::scaleIf(const T_Pt& pt) const
{
    return scale_ ? scale(pt) : pt;
}

// Explicit template instantiations for the used types to avoid linker errors
template DrawPoint Window::scale(const DrawPoint&);
template Extent Window::scale(const Extent&);
template DrawPoint Window::scaleIf(const DrawPoint&) const;
template Extent Window::scaleIf(const Extent&) const;

bool Window::isInLockedRegion(const Position& pos, const Window* exception) const
{
    for(const auto& lockEntry : lockedAreas_)
    {
        // Ignore exception
        if(lockEntry.first == exception)
            continue;
        if(isPointInRect(pos, lockEntry.second))
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
    return isPointInRect(mousePos.pos, getBoundaryRect());
}
