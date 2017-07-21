// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "Window.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "controls/controls.h"
#include "RescaleWindowProp.h"
#include "drivers/ScreenResizeEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "driver/src/MouseCoords.h"
#include "ExtensionList.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>
#include <cstdarg>

Window::Window(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size):
    parent_(parent), id_(id), pos_(pos), size_(size), active_(false), visible_(true), scale_(false), isInMouseRelay(false), animations_(this)
{
}

Window::~Window()
{
    RTTR_Assert(!isInMouseRelay);
    // Steuerelemente aufräumen
    BOOST_FOREACH(Window* ctrl, childIdToWnd_ | boost::adaptors::map_values)
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

Extent Window::GetSize(bool scale /*= false*/) const
{
    return scale ? Scale(size_) : size_;
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
    BOOST_FOREACH(Window* wnd, childIdToWnd_ | boost::adaptors::map_values)
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
    BOOST_REVERSE_FOREACH(Window* wnd, childIdToWnd_ | boost::adaptors::map_values)
    {
        if(!lockedAreas_.empty() && TestWindowInRegion(wnd, mc))
            continue;

        if(wnd->visible_ && wnd->active_ && CALL_MEMBER_FN(*wnd, msg)(mc))
            processed = true;
    }

    for(std::vector<Window*>::iterator it = tofreeAreas_.begin(); it != tofreeAreas_.end(); ++it)
        lockedAreas_.erase(*it);
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
    for(std::map<unsigned int, Window*>::iterator it = childIdToWnd_.begin(); it != childIdToWnd_.end(); ++it)
        it->second->SetActive(activate);
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
    std::vector<Window*>::iterator it = std::find(tofreeAreas_.begin(), tofreeAreas_.end(), window);
    if(it != tofreeAreas_.end())
        tofreeAreas_.erase(it);
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
    std::map<unsigned int, Window*>::iterator it = childIdToWnd_.find(id);

    if(it == childIdToWnd_.end())
        return;

    delete it->second;

    childIdToWnd_.erase(it);
}

ctrlBuildingIcon* Window::AddBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, const Nation nation, unsigned short size, const std::string& tooltip)
{
    return AddCtrl(new ctrlBuildingIcon(this, id, ScaleIf(pos), type, nation, ScaleIf(Extent(size, 0)).x, tooltip));
}

ctrlButton* Window::AddTextButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc, const std::string& text,  glArchivItem_Font* font, const std::string& tooltip)
{
    return AddCtrl(new ctrlTextButton(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, tooltip));
}

ctrlButton* Window::AddColorButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc, const unsigned fillColor, const std::string& tooltip)
{
    return AddCtrl(new ctrlColorButton(this, id, ScaleIf(pos), ScaleIf(size), tc, fillColor, tooltip));
}

ctrlButton* Window::AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc, glArchivItem_Bitmap* const image,  const std::string& tooltip)
{
    return AddCtrl(new ctrlImageButton(this, id, ScaleIf(pos), ScaleIf(size), tc, image, tooltip));
}

ctrlChat* Window::AddChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font)
{
    return AddCtrl(new ctrlChat(this, id, ScaleIf(pos), ScaleIf(size), tc, font));
}

ctrlCheck* Window::AddCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& text, glArchivItem_Font* font, bool readonly)
{
    return AddCtrl(new ctrlCheck(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, readonly));
}

ctrlComboBox* Window::AddComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned short max_list_height, bool readonly)
{
    return AddCtrl(new ctrlComboBox(this, id, ScaleIf(pos), ScaleIf(size), tc, font, max_list_height, readonly));
}

ctrlDeepening* Window::AddTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& text, glArchivItem_Font* font, unsigned color)
{
    return AddCtrl(new ctrlTextDeepening(this, id, ScaleIf(pos), ScaleIf(size), tc, text, font, color));
}

ctrlDeepening* Window::AddColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned fillColor)
{
    return AddCtrl(new ctrlColorDeepening(this, id, ScaleIf(pos), ScaleIf(size), tc, fillColor));
}

ctrlEdit* Window::AddEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned short maxlength, bool password, bool disabled, bool notify)
{
    return AddCtrl(new ctrlEdit(this, id, ScaleIf(pos), ScaleIf(size), tc, font, maxlength, password, disabled, notify));
}

ctrlGroup* Window::AddGroup(unsigned id)
{
    return AddCtrl(new ctrlGroup(this, id));
}

ctrlImage* Window::AddImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip)
{
    return AddCtrl(new ctrlImage(this, id, ScaleIf(pos), image, tooltip));
}

ctrlList* Window::AddList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font)
{
    return AddCtrl(new ctrlList(this, id, ScaleIf(pos), ScaleIf(size), tc, font));
}

ctrlMultiline* Window::AddMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned format)
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
ctrlOptionGroup* Window::AddOptionGroup(unsigned id, int select_type)
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
ctrlMultiSelectGroup* Window::AddMultiSelectGroup(unsigned id, int select_type)
{
    return AddCtrl(new ctrlMultiSelectGroup(this, id, select_type));
}

ctrlPercent* Window::AddPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned text_color, glArchivItem_Font* font, const unsigned short* percentage)
{
    return AddCtrl(new ctrlPercent(this, id, ScaleIf(pos), ScaleIf(size), tc, text_color, font, percentage));
}

ctrlProgress* Window::AddProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned short button_minus, unsigned short button_plus, unsigned short maximum, const std::string& tooltip, const Extent& padding, unsigned force_color, const std::string& button_minus_tooltip, const std::string& button_plus_tooltip)
{
    return AddCtrl(new ctrlProgress(this, id, ScaleIf(pos), ScaleIf(size), tc, button_minus, button_plus, maximum, padding, force_color, tooltip, button_minus_tooltip, button_plus_tooltip));
}

ctrlScrollBar* Window::AddScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height, TextureColor tc, unsigned short page_size)
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
ctrlTable* Window::AddTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned columns,
                            ...)
{
    ctrlTable* ctrl;
    va_list liste;
    va_start(liste, columns);

    ctrl = new ctrlTable(this, id, ScaleIf(pos), ScaleIf(size), tc, font, columns, liste);

    va_end(liste);

    return AddCtrl(ctrl);
}

ctrlTimer* Window::AddTimer(unsigned id, unsigned timeout)
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
 *                      @p glArchivItem_Font::DF_LEFT    - Text links ( standard )
 *                      @p glArchivItem_Font::DF_CENTER  - Text mittig
 *                      @p glArchivItem_Font::DF_RIGHT   - Text rechts
 *                      @p glArchivItem_Font::DF_TOP     - Text oben ( standard )
 *                      @p glArchivItem_Font::DF_VCENTER - Text vertikal zentriert
 *                      @p glArchivItem_Font::DF_BOTTOM  - Text unten
 *  @param[in] font   Schriftart
 */
ctrlText* Window::AddText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, unsigned format, glArchivItem_Font* font)
{
    return AddCtrl(new ctrlText(this, id, ScaleIf(pos), text, color, format, font));
}

ctrlVarDeepening* Window::AddVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& formatstr, glArchivItem_Font* font, unsigned color, unsigned parameters,
        ...)
{
    va_list liste;
    va_start(liste, parameters);

    ctrlVarDeepening* ctrl = new ctrlVarDeepening(this, id, ScaleIf(pos), ScaleIf(size), tc, formatstr, font, color, parameters, liste);

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
 *                          @p glArchivItem_Font::DF_LEFT    - Text links ( standard )
 *                          @p glArchivItem_Font::DF_CENTER  - Text mittig
 *                          @p glArchivItem_Font::DF_RIGHT   - Text rechts
 *                          @p glArchivItem_Font::DF_TOP     - Text oben ( standard )
 *                          @p glArchivItem_Font::DF_VCENTER - Text vertikal zentriert
 *                          @p glArchivItem_Font::DF_BOTTOM  - Text unten
 *  @param[in] font       Schriftart
 *  @param[in] parameters Anzahl der nachfolgenden Parameter
 *  @param[in] ...        die variablen Parameter
 */
ctrlVarText* Window::AddVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color, unsigned format, glArchivItem_Font* font, unsigned parameters,
                                ...)
{
    va_list liste;
    va_start(liste, parameters);

    ctrlVarText* ctrl = new ctrlVarText(this, id, ScaleIf(pos), formatstr, color, format, font, parameters, liste);

    va_end(liste);

    return AddCtrl(ctrl);
}

ctrlPreviewMinimap* Window::AddPreviewMinimap(const unsigned id, const DrawPoint& pos, const Extent& size, glArchivItem_Map* const map)
{
    return AddCtrl(new ctrlPreviewMinimap(this, id, ScaleIf(pos), ScaleIf(size), map));
}

/**
 *  Zeichnet einen 3D-Rahmen.
 */
void Window::Draw3D(const Rect& rect, TextureColor tc, unsigned short type, bool illuminated, bool drawContent, unsigned color)
{
    if(rect.getSize().x < 4 || rect.getSize().y < 4 || tc == TC_INVISIBLE)
        return;

    DrawPoint origin = rect.getOrigin();
    // Position of the horizontal and vertical image border
    DrawPoint horImgBorderPos(origin);
    DrawPoint vertImgBorderPos(origin);

    if(type > 1)
    {
        // For deepened effect the img border is at bottom and right
        // else it stays top and left
        horImgBorderPos += DrawPoint(0, rect.getSize().y - 2);
        vertImgBorderPos += DrawPoint(rect.getSize().x - 2, 0);
    }
    // Draw img borders
    glArchivItem_Bitmap* borderImg = LOADER.GetImageN("io", 12 + tc);
    borderImg->DrawPart(Rect(horImgBorderPos, Extent(rect.getSize().x, 2)));
    borderImg->DrawPart(Rect(vertImgBorderPos, Extent(2, rect.getSize().y)));

    // Draw black borders over the img borders
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_STRIP);
    // Left lower point
    DrawPoint lbPt = rect.getOrigin() + DrawPoint(rect.getSize());
    if(type <= 1)
    {
        // Bottom line with edge in left top and right line with little edge on left top
        glVertex2i(lbPt.x, origin.y);
        glVertex2i(lbPt.x - 2, origin.y + 1);
        glVertex2i(lbPt.x, lbPt.y);
        glVertex2i(lbPt.x - 2, lbPt.y - 2);
        glVertex2i(origin.x, lbPt.y);
        glVertex2i(origin.x + 1, lbPt.y - 2);
    } else
    {
        // Top line with edge on right and left line with edge on bottom
        glVertex2i(origin.x, lbPt.y);
        glVertex2i(origin.x + 2, lbPt.y - 1);
        glVertex2i(origin.x, origin.y);
        glVertex2i(origin.x + 2, origin.y + 2);
        glVertex2i(lbPt.x, origin.y);
        glVertex2i(lbPt.x - 1, origin.y + 2);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);

    if(!drawContent)
        return;

    if(illuminated)
    {
        // Modulate2x anmachen
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);
    }

    DrawPoint contentPos = origin + DrawPoint(2, 2);
    Extent contentSize(rect.getSize() - Extent(4, 4));
    DrawPoint contentOffset(0, 0);
    if(type <= 1)
    {
        // Move the content a bit to left upper for non-deepened version
        contentOffset = DrawPoint(2, 2);
    }
    unsigned texture = (type == 1) ? tc * 2 : tc * 2 + 1;
    LOADER.GetImageN("io", texture)->DrawPart(Rect(contentPos, contentSize), contentOffset, color);

    if(illuminated)
    {
        // Modulate2x wieder ausmachen
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
}
/**
 *  zeichnet ein Rechteck.
 *
 *  @param[in] x X-Koordinate
 */
void Window::DrawRectangle(const Rect& rect, unsigned color)
{
    glDisable(GL_TEXTURE_2D);

    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));

    glBegin(GL_QUADS);
    glVertex2i(rect.left, rect.top);
    glVertex2i(rect.left, rect.bottom);
    glVertex2i(rect.right, rect.bottom);
    glVertex2i(rect.right, rect.top);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

/**
 *  zeichnet eine Linie.
 *
 *  @param[in] x X-Koordinate
 */
void Window::DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color)
{
    glDisable(GL_TEXTURE_2D);
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));

    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2i(pt1.x, pt1.y);
    glVertex2i(pt2.x, pt2.y);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

void Window::Msg_PaintBefore()
{
    animations_.update(VIDEODRIVER.GetTickCount());
}

void Window::Msg_ScreenResize(const ScreenResizeEvent& sr)
{
    // If the window elements don't get scaled there is nothing to do
    if(!scale_)
        return;
    RescaleWindowProp rescale(sr.oldSize, sr.newSize);
    BOOST_FOREACH(Window* ctrl, childIdToWnd_ | boost::adaptors::map_values)
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
T_Pt Window::Scale(const T_Pt& pt) const
{
    return ScaleWindowPropUp::scale(pt, VIDEODRIVER.GetScreenSize());
}

template<class T_Pt>
T_Pt Window::ScaleIf(const T_Pt& pt) const
{
    return scale_ ? Scale(pt) : pt;
}


/**
 *  zeichnet die Steuerelemente.
 */
void Window::DrawControls()
{
    BOOST_FOREACH(Window* control, childIdToWnd_ | boost::adaptors::map_values)
        control->Msg_PaintBefore();
    BOOST_FOREACH(Window* control, childIdToWnd_ | boost::adaptors::map_values)
        control->Draw();
    BOOST_FOREACH(Window* control, childIdToWnd_ | boost::adaptors::map_values)
        control->Msg_PaintAfter();
}

/**
 *  prüft ob Mauskoordinaten in einer gesperrten Region liegt.
 *
 *  @param[in] window das Fenster, welches die Region sperrt.
 *  @param[in] mc     Mauskoordinaten.
 *
 *  @return @p true falls Mausposition innerhalb der gesperrten Region,
 *          @p false falls außerhalb
 */
bool Window::TestWindowInRegion(Window* window, const MouseCoords& mc) const
{
    for(std::map<Window*, Rect>::const_iterator it = lockedAreas_.begin(); it != lockedAreas_.end(); ++it)
    {
        if(it->first == window)
            continue; // Locking window can always access its locked regions
        // All others cannot:
        if(IsPointInRect(mc.GetPos(), it->second))
            return true;
    }
    return false;
}
