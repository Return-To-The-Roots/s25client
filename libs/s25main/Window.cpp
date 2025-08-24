// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
    // Steuerelemente aufräumen
    for(Window* ctrl : childIdToWnd_ | boost::adaptors::map_values)
        delete ctrl;
}

/**
 *  zeichnet das Fenster.
 */
void Window::Draw()
{
    if(visible_)
        Draw_();
}

DrawPoint Window::GetPos() const
{
    return pos_;
}

DrawPoint Window::GetDrawPos() const
{
    DrawPoint result = pos_;
    const Window* temp = this;

    // Relative Koordinaten in absolute umrechnen
    // ( d.h. Koordinaten von allen Eltern zusammenaddieren )
    while(temp->parent_)
    {
        temp = temp->parent_;
        result += temp->pos_;
    }

    return result;
}

Extent Window::GetSize() const
{
    return size_;
}

Rect Window::GetDrawRect() const
{
    return Rect(GetDrawPos(), GetSize());
}

Rect Window::GetBoundaryRect() const
{
    // Default to draw rect
    return GetDrawRect();
}

/**
 *  Sendet eine Fensternachricht an die Steuerelemente.
 *
 *  @param[in] msg   Die Nachricht.
 *  @param[in] id    Die ID des Quellsteuerelements.
 *  @param[in] param Ein nachrichtenspezifischer Parameter.
 */
bool Window::RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke)
{
    // Abgeleitete Klassen fragen, ob das Weiterleiten von Nachrichten erlaubt ist
    // (IngameFenster könnten ja z.B. minimiert sein)
    if(!IsMessageRelayAllowed())
        return false;

    // Alle Controls durchgehen
    // Falls das Fenster dann plötzlich nich mehr aktiv ist (z.b. neues Fenster geöffnet, sofort abbrechen!)
    for(Window* wnd : childIdToWnd_ | boost::adaptors::map_values)
    {
        if(wnd->visible_ && wnd->active_ && CALL_MEMBER_FN(*wnd, msg)(ke))
            return true;
    }

    return false;
}

bool Window::RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc)
{
    // Abgeleitete Klassen fragen, ob das Weiterleiten von Mausnachrichten erlaubt ist
    // (IngameFenster könnten ja z.B. minimiert sein)
    if(!IsMessageRelayAllowed())
        return false;

    bool processed = false;
    isInMouseRelay = true;

    // Alle Controls durchgehen
    // Use reverse iterator because the topmost (=last elements) should receive the messages first!
    for(Window* wnd : childIdToWnd_ | boost::adaptors::map_values | boost::adaptors::reversed)
    {
        if(!lockedAreas_.empty() && IsInLockedRegion(mc.GetPos(), wnd))
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
void Window::SetActive(bool activate)
{
    this->active_ = activate;
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
        it.second->SetActive(activate);
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
    if(GetParent())
        GetParent()->LockRegion(this, rect);
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
    if(GetParent())
        GetParent()->FreeRegion(this);
}

void Window::SetPos(const DrawPoint& newPos)
{
    pos_ = newPos;
}

/// Weiterleitung von Nachrichten von abgeleiteten Klassen erlaubt oder nicht?
bool Window::IsMessageRelayAllowed() const
{
    return true;
}

void Window::DeleteCtrl(unsigned id)
{
    auto it = childIdToWnd_.find(id);

    if(it == childIdToWnd_.end())
        return;

    delete it->second;

    childIdToWnd_.erase(it);
}

ctrlBuildingIcon* Window::AddBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, const Nation nation,
                                          unsigned short size, const std::string& tooltip)
{
    return AddCtrl(new ctrlBuildingIcon(this, id, ScaleIf(pos), type, nation, ScaleIf(Extent(size, 0)).x, tooltip));
}

ctrlButton* Window::AddTextButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                  const std::string& text, const glFont* font, const std::string& tooltip)
{
    return AddCtrl(new ctrlTextButton(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, tooltip));
}

ctrlButton* Window::AddColorButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   const unsigned fillColor, const std::string& tooltip)
{
    return AddCtrl(new ctrlColorButton(this, id, ScaleIf(pos), ScaleIf(size), tc, fillColor, tooltip));
}

ctrlButton* Window::AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   ITexture* const image, const std::string& tooltip)
{
    return AddCtrl(new ctrlImageButton(this, id, ScaleIf(pos), ScaleIf(size), tc, image, tooltip));
}

ctrlButton* Window::AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                   glArchivItem_Bitmap* const image, const std::string& tooltip)
{
    return AddImageButton(id, pos, size, tc, static_cast<ITexture*>(image), tooltip);
}

ctrlChat* Window::AddChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const glFont* font)
{
    return AddCtrl(new ctrlChat(this, id, ScaleIf(pos), ScaleIf(size), tc, font));
}

ctrlCheck* Window::AddCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               const std::string& text, const glFont* font, bool readonly)
{
    return AddCtrl(new ctrlCheck(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, readonly));
}

ctrlComboBox* Window::AddComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                  const glFont* font, unsigned short max_list_height, bool readonly)
{
    return AddCtrl(new ctrlComboBox(this, id, ScaleIf(pos), ScaleIf(size), tc, font, max_list_height, readonly));
}

ctrlDeepening* Window::AddTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                        const std::string& text, const glFont* font, unsigned color, FontStyle style)
{
    return AddCtrl(new ctrlTextDeepening(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, color, style));
}

ctrlDeepening* Window::AddColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                         unsigned fillColor)
{
    return AddCtrl(new ctrlColorDeepening(this, id, ScaleIf(pos), ScaleIf(size), tc, fillColor));
}

ctrlEdit* Window::AddEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                          unsigned short maxlength, bool password, bool disabled, bool notify)
{
    return AddCtrl(
      new ctrlEdit(this, id, ScaleIf(pos), ScaleIf(size), tc, font, maxlength, password, disabled, notify));
}

ctrlGroup* Window::AddGroup(unsigned id)
{
    return AddCtrl(new ctrlGroup(this, id));
}

ctrlImage* Window::AddImage(unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip)
{
    return AddCtrl(new ctrlImage(this, id, ScaleIf(pos), image, tooltip));
}

ctrlImage* Window::AddImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip)
{
    return AddImage(id, pos, static_cast<ITexture*>(image), tooltip);
}

ctrlList* Window::AddList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font)
{
    return AddCtrl(new ctrlList(this, id, ScaleIf(pos), ScaleIf(size), tc, font));
}

ctrlMultiline* Window::AddMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                    const glFont* font, FontStyle format)
{
    return AddCtrl(new ctrlMultiline(this, id, ScaleIf(pos), ScaleIf(size), tc, font, format));
}

/**
 *  fügt ein OptionenGruppe hinzu.
 *
 *  @param[in] id          ID des Steuerelements
 *  @param[in] select_type Typ der Auswahl
 *
 *  @return Instanz das Steuerelement.
 */
ctrlOptionGroup* Window::AddOptionGroup(unsigned id, GroupSelectType select_type)
{
    return AddCtrl(new ctrlOptionGroup(this, id, select_type));
}

/**
 *  fügt ein MultiSelectGruppe hinzu.
 *
 *  @param[in] id          ID des Steuerelements
 *  @param[in] select_type Typ der Auswahl
 *
 *  @return Instanz das Steuerelement.
 */
ctrlMultiSelectGroup* Window::AddMultiSelectGroup(unsigned id, GroupSelectType select_type)
{
    return AddCtrl(new ctrlMultiSelectGroup(this, id, select_type));
}

ctrlPercent* Window::AddPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                unsigned text_color, const glFont* font, const unsigned short* percentage)
{
    return AddCtrl(new ctrlPercent(this, id, ScaleIf(pos), ScaleIf(size), tc, text_color, font, percentage));
}

ctrlProgress* Window::AddProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                  unsigned short button_minus, unsigned short button_plus, unsigned short maximum,
                                  const std::string& tooltip, const Extent& padding, unsigned force_color,
                                  const std::string& button_minus_tooltip, const std::string& button_plus_tooltip)
{
    return AddCtrl(new ctrlProgress(this, id, ScaleIf(pos), ScaleIf(size), tc, button_minus, button_plus, maximum,
                                    padding, force_color, tooltip, button_minus_tooltip, button_plus_tooltip));
}

ctrlScrollBar* Window::AddScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height,
                                    TextureColor tc, unsigned short page_size)
{
    button_height = ScaleIf(Extent(0, button_height)).y;

    return AddCtrl(new ctrlScrollBar(this, id, ScaleIf(pos), ScaleIf(size), button_height, tc, page_size));
}

ctrlTab* Window::AddTabCtrl(unsigned id, const DrawPoint& pos, unsigned short width)
{
    return AddCtrl(new ctrlTab(this, id, ScaleIf(pos), ScaleIf(Extent(width, 0)).x));
}

/**
 *  fügt eine Tabelle hinzu.
 *  ... sollte eine Menge von const char*, int und SortType sein
 */
ctrlTable* Window::AddTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                            std::vector<TableColumn> columns)
{
    return AddCtrl(new ctrlTable(this, id, ScaleIf(pos), ScaleIf(size), tc, font, std::move(columns)));
}

ctrlTimer* Window::AddTimer(unsigned id, std::chrono::milliseconds timeout)
{
    return AddCtrl(new ctrlTimer(this, id, timeout));
}

/**
 *  fügt ein TextCtrl hinzu.
 *
 *  @param[in] x      X-Koordinate des Steuerelements
 *  @param[in] y      Y-Koordinate des Steuerelements
 *  @param[in] text   Text
 *  @param[in] color  Textfarbe
 *  @param[in] format Formatierung des Textes
 *                      @p FontStyle::LEFT    - Text links ( standard )
 *                      @p FontStyle::CENTER  - Text mittig
 *                      @p FontStyle::RIGHT   - Text rechts
 *                      @p FontStyle::TOP     - Text oben ( standard )
 *                      @p FontStyle::VCENTER - Text vertikal zentriert
 *                      @p FontStyle::BOTTOM  - Text unten
 *  @param[in] font   Schriftart
 */
ctrlText* Window::AddText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                          const glFont* font)
{
    return AddCtrl(new ctrlText(this, id, ScaleIf(pos), text, color, format, font));
}

ctrlMapSelection* Window::AddMapSelection(unsigned id, const DrawPoint& pos, const Extent& size,
                                          const SelectionMapInputData& inputData)
{
    return AddCtrl(new ctrlMapSelection(this, id, ScaleIf(pos), ScaleIf(size), inputData));
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

    auto* ctrl =
      new ctrlVarDeepening(this, id, ScaleIf(pos), ScaleIf(size), tc, formatstr, font, color, parameters, liste);

    va_end(liste);

    return AddCtrl(ctrl);
}

/**
 *  fügt ein variables TextCtrl hinzu.
 *
 *  @param[in] x          X-Koordinate des Steuerelements
 *  @param[in] y          Y-Koordinate des Steuerelements
 *  @param[in] formatstr  Der Formatstring des Steuerelements
 *  @param[in] color      Textfarbe
 *  @param[in] format     Formatierung des Textes
 *                          @p FontStyle::LEFT    - Text links ( standard )
 *                          @p FontStyle::CENTER  - Text mittig
 *                          @p FontStyle::RIGHT   - Text rechts
 *                          @p FontStyle::TOP     - Text oben ( standard )
 *                          @p FontStyle::VCENTER - Text vertikal zentriert
 *                          @p FontStyle::BOTTOM  - Text unten
 *  @param[in] font       Schriftart
 *  @param[in] parameters Anzahl der nachfolgenden Parameter
 *  @param[in] ...        die variablen Parameter
 */
ctrlVarText* Window::AddVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                                FontStyle format, const glFont* font, unsigned parameters, ...)
{
    va_list liste;
    va_start(liste, parameters);

    auto* ctrl = new ctrlVarText(this, id, ScaleIf(pos), formatstr, color, format, font, parameters, liste);

    va_end(liste);

    return AddCtrl(ctrl);
}

ctrlPreviewMinimap* Window::AddPreviewMinimap(const unsigned id, const DrawPoint& pos, const Extent& size,
                                              libsiedler2::ArchivItem_Map* const map)
{
    return AddCtrl(new ctrlPreviewMinimap(this, id, ScaleIf(pos), ScaleIf(size), map));
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
    glArchivItem_Bitmap* borderImg = LOADER.GetImageN("io", 12 + rttr::enum_cast(tc));
    VIDEODRIVER.GetRenderer()->Draw3DBorder(rect, elevated, *borderImg);
}

void Window::Draw3DContent(const Rect& rect, TextureColor tc, bool elevated, bool highlighted, bool illuminated,
                           unsigned contentColor)
{
    if(tc == TextureColor::Invisible)
        return;
    glArchivItem_Bitmap* contentImg = LOADER.GetImageN("io", rttr::enum_cast(tc) * 2 + (highlighted ? 0 : 1));
    VIDEODRIVER.GetRenderer()->Draw3DContent(rect, elevated, *contentImg, illuminated, contentColor);
}

void Window::DrawRectangle(const Rect& rect, unsigned color)
{
    VIDEODRIVER.GetRenderer()->DrawRect(rect, color);
}

void Window::DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color)
{
    VIDEODRIVER.GetRenderer()->DrawLine(pt1, pt2, width, color);
}

void Window::Msg_PaintBefore()
{
    animations_.update(VIDEODRIVER.GetTickCount());
    for(Window* control : childIdToWnd_ | boost::adaptors::map_values)
        control->Msg_PaintBefore();
}

void Window::Msg_PaintAfter()
{
    for(Window* control : childIdToWnd_ | boost::adaptors::map_values)
        control->Msg_PaintAfter();
}

void Window::Draw_()
{
    for(Window* control : childIdToWnd_ | boost::adaptors::map_values)
        control->Draw();
}

void Window::Msg_ScreenResize(const ScreenResizeEvent& sr)
{
    // If the window elements don't get scaled there is nothing to do
    if(!scale_)
        return;
    RescaleWindowProp rescale(sr.oldSize, sr.newSize);
    for(Window* ctrl : childIdToWnd_ | boost::adaptors::map_values)
    {
        if(!ctrl)
            continue;
        // Save new size (could otherwise be changed(?) in Msg_ScreenResize)
        Extent newSize = rescale(ctrl->GetSize());
        ctrl->SetPos(rescale(ctrl->GetPos()));
        ctrl->Msg_ScreenResize(sr);
        ctrl->Resize(newSize);
    }
    animations_.onRescale(sr);
}

template<class T_Pt>
T_Pt Window::Scale(const T_Pt& pt)
{
    return ScaleWindowPropUp::scale(pt, VIDEODRIVER.GetRenderSize());
}

template<class T_Pt>
T_Pt Window::ScaleIf(const T_Pt& pt) const
{
    return scale_ ? Scale(pt) : pt;
}

// Inlining removes those. so add it here
template DrawPoint Window::ScaleIf(const DrawPoint&) const;
template Extent Window::ScaleIf(const Extent&) const;

bool Window::IsInLockedRegion(const Position& pos, const Window* exception) const
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

bool Window::IsMouseOver() const
{
    return IsMouseOver(VIDEODRIVER.GetMousePos());
}

bool Window::IsMouseOver(const MouseCoords& mousePos) const
{
    return IsPointInRect(mousePos.GetPos(), GetDrawRect());
}
