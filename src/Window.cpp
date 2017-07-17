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

Window::Window():
    pos_(0, 0), width_(0), height_(0), id_(0), parent_(NULL), active_(false), visible_(true), scale_(false), tooltip_(""),
    isInMouseRelay(false), animations_(this)
{
}

/**
 *  Konstruktor von @p Window mit Parametern.
 *
 *  @param[in] x      X-Position des Fensters.
 *  @param[in] y      Y-Position des Fensters.
 *  @param[in] id     ID des Fensters
 *  @param[in] parent Handle auf das Parentfenster.
 */
Window::Window(const DrawPoint& pos,
               unsigned int id,
               Window* parent,
               unsigned short width,
               unsigned short height,
               const std::string& tooltip):
    pos_(pos), width_(width), height_(height), id_(id), parent_(parent), active_(false), visible_(true), scale_(false), tooltip_(tooltip),
    isInMouseRelay(false), animations_(this)
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

/**
 *  liefert die X-Koordinate.
 *
 *  @param[in] absolute Absolute Koordinate oder relative?
 *
 *  @return die X-Koordinate.
 */
DrawPoint::ElementType Window::GetX(bool absolute) const
{
    if(!absolute)
        return pos_.x;

    DrawPoint::ElementType abs_x = pos_.x;
    const Window* temp = this;

    // Relative Koordinaten in absolute umrechnen
    // ( d.h. Koordinaten von allen Eltern zusammenaddieren )
    while(temp->parent_)
    {
        temp = temp->parent_;
        abs_x += temp->pos_.x;
    }

    return abs_x;
}

/**
 *  liefert die Y-Koordinate.
 *
 *  @param[in] absolute Absolute Koordinate oder relative?
 *
 *  @return die Y-Koordinate.
 */
DrawPoint::ElementType Window::GetY(bool absolute) const
{
    if(!absolute)
        return pos_.y;

    DrawPoint::ElementType abs_y = pos_.y;
    const Window* temp = this;

    // Relative Koordinaten in absolute umrechnen
    // ( d.h. Koordinaten von allen Eltern zusammenaddieren )
    while(temp->parent_)
    {
        temp = temp->parent_;
        abs_y += temp->pos_.y;
    }

    return abs_y;
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

unsigned short Window::GetWidth(const bool scale /*= false*/) const
{
    return (scale) ? ScaleX(width_) : width_;
}

unsigned short Window::GetHeight(const bool scale /*= false*/) const
{
    return (scale) ? ScaleY(height_) : height_;
}

Extent Window::GetSize(bool scale /*= false*/) const
{
    return Extent(GetWidth(scale), GetHeight(scale));
}

Rect Window::GetDrawRect() const
{
    return Rect(GetDrawPos(), GetWidth(), GetHeight());
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

void Window::Move(const DrawPoint& offsetOrPos, bool absolute /*= true*/)
{
    if(absolute)
        pos_ = offsetOrPos;
    else
        pos_ += offsetOrPos;
}

/// Weiterleitung von Nachrichten von abgeleiteten Klassen erlaubt oder nicht?
bool Window::IsMessageRelayAllowed() const
{
    return true;
}

void Window::DeleteCtrl(unsigned int id)
{
    std::map<unsigned int, Window*>::iterator it = childIdToWnd_.find(id);

    if(it == childIdToWnd_.end())
        return;

    delete it->second;

    childIdToWnd_.erase(it);
}

/**
 *  fügt ein BuildingIcon hinzu.
 */
ctrlBuildingIcon* Window::AddBuildingIcon(unsigned int id,
        unsigned short x,
        unsigned short y,
        BuildingType type,
        const Nation nation,
        unsigned short size,
        const std::string& tooltip)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
    }

    return AddCtrl(id, new ctrlBuildingIcon(this, id, x, y, type, nation, size, tooltip));
}

/**
 *  fügt einen ButtonCtrl hinzu.
 *
 *  @param[in] x      X-Koordinate des Steuerelements
 *  @param[in] y      Y-Koordinate des Steuerelements
 *  @param[in] width  Breite des Steuerelements
 *  @param[in] height Höhe des Steuerelements
 *  @param[in] tc     Farbe des Steuerelements
 *  @param[in] type   Typ des Steuerelements (@p false für Text, @p true für Bild)
 *  @param[in] text   Text des Buttons (nur für @p type gleich @p false)
 *  @param[in] font   Schrift des Buttons (nur für @p type gleich @p false)
 *  @param[in] image  Bild des Buttons (nur für @p type gleich @p true)
 *  @param[in] border Soll der Button einen Rahmen haben?
 */


/// fügt einen Text-ButtonCtrl hinzu.
ctrlTextButton* Window::AddTextButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const TextureColor tc, const std::string& text,  glArchivItem_Font* font, const std::string& tooltip)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlTextButton(this, id, x, y, width, height, tc, text, font, tooltip));
}

/// fügt einen Color-ButtonCtrl hinzu.
ctrlColorButton* Window::AddColorButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const TextureColor tc, const unsigned int fillColor, const std::string& tooltip)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlColorButton(this, id, x, y, width, height, tc, fillColor, tooltip));
}


/// fügt einen Image-ButtonCtrl hinzu.
ctrlImageButton* Window::AddImageButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const TextureColor tc, glArchivItem_Bitmap* const image,  const std::string& tooltip)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlImageButton(this, id, x, y, width, height, tc, image, tooltip));
}


/**
 *  fügt ein ChatCtrl hinzu.
 */
ctrlChat* Window::AddChatCtrl(unsigned int id,
                              unsigned short x,
                              unsigned short y,
                              unsigned short width,
                              unsigned short height,
                              TextureColor tc,
                              glArchivItem_Font* font)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlChat(this, id, x, y, width, height, tc, font));
}

/**
 *  fügt eine Checkbox hinzu.
 */
ctrlCheck* Window::AddCheckBox(unsigned int id,
                               unsigned short x,
                               unsigned short y,
                               unsigned short width,
                               unsigned short height,
                               TextureColor tc,
                               const std::string& text,
                               glArchivItem_Font* font,
                               bool readonly)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlCheck(this, id, x, y, width, height, tc, text, font, readonly));
}

/**
 *  fügt eine Combobox hinzu.
 */
ctrlComboBox* Window::AddComboBox(unsigned int id,
                                  unsigned short x,
                                  unsigned short y,
                                  unsigned short width,
                                  unsigned short height,
                                  TextureColor tc,
                                  glArchivItem_Font* font,
                                  unsigned short max_list_height,
                                  bool readonly)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlComboBox(this, id, x, y, width, height, tc, font, max_list_height, readonly));
}

/**
 *  fügt ein vertieftes TextCtrl hinzu.
 */
ctrlTextDeepening* Window::AddTextDeepening(unsigned int id,
                                    unsigned short x,
                                    unsigned short y,
                                    unsigned short width,
                                    unsigned short height,
                                    TextureColor tc,
                                    const std::string& text,
                                    glArchivItem_Font* font,
                                    unsigned int color)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlTextDeepening(this, id, DrawPoint(x, y), width, height, tc, text, font, color));
}

/**
 *  adds a colored Deepening
 */
ctrlColorDeepening* Window::AddColorDeepening(unsigned int id,
        unsigned short x,
        unsigned short y,
        unsigned short width,
        unsigned short height,
        TextureColor tc,
        unsigned int fillColor)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlColorDeepening(this, id, DrawPoint(x, y), width, height, tc, fillColor));
}

/**
 *  fügt ein EditCtrl hinzu.
 */
ctrlEdit* Window::AddEdit(unsigned int id,
                          unsigned short x,
                          unsigned short y,
                          unsigned short width,
                          unsigned short height,
                          TextureColor tc,
                          glArchivItem_Font* font,
                          unsigned short maxlength,
                          bool password,
                          bool disabled,
                          bool notify)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlEdit(this, id, x, y, width, height, tc, font, maxlength, password, disabled, notify));
}

/**
 *  fügt eine Gruppe hinzu.
 */
ctrlGroup* Window::AddGroup(unsigned int id, bool scale)
{
    return AddCtrl(id, new ctrlGroup(this, id, scale));
}

/**
 *  fügt ein ImageCtrl hinzu.
 */
ctrlImage* Window::AddImage(unsigned int id,
                            unsigned short x,
                            unsigned short y,
                            glArchivItem_Bitmap* image, const std::string& tooltip)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
    }

    return AddCtrl(id, new ctrlImage(this, id, x, y, image, tooltip));
}

/**
 *  fügt ein ListCtrl hinzu.
 */
ctrlList* Window::AddList(unsigned int id,
                          unsigned short x,
                          unsigned short y,
                          unsigned short width,
                          unsigned short height,
                          TextureColor tc,
                          glArchivItem_Font* font)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlList(this, id, x, y, width, height, tc, font));
}

/**
 *  fügt ein mehrzeiliges TextCtrl hinzu.
 */
ctrlMultiline* Window::AddMultiline(unsigned int id,
                                    unsigned short x,
                                    unsigned short y,
                                    unsigned short width,
                                    unsigned short height,
                                    TextureColor tc,
                                    glArchivItem_Font* font,
                                    unsigned int format)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlMultiline(this, id, DrawPoint(x, y), width, height, tc, font, format));
}

/**
 *  fügt ein OptionenGruppe hinzu.
 *
 *  @param[in] id          ID des Steuerelements
 *  @param[in] select_type Typ der Auswahl
 *
 *  @return Instanz das Steuerelement.
 */
ctrlOptionGroup* Window::AddOptionGroup(unsigned int id,
                                        int select_type,
                                        bool scale)
{
    return AddCtrl(id, new ctrlOptionGroup(this, id, select_type, scale));
}

/**
 *  fügt ein MultiSelectGruppe hinzu.
 *
 *  @param[in] id          ID des Steuerelements
 *  @param[in] select_type Typ der Auswahl
 *
 *  @return Instanz das Steuerelement.
 */
ctrlMultiSelectGroup* Window::AddMultiSelectGroup(unsigned int id,
        int select_type,
        bool scale)
{
    return AddCtrl(id, new ctrlMultiSelectGroup(this, id, select_type, scale));
}

/**
 *  fügt eine prozentuale ProgressBar hinzu.
 */
ctrlPercent* Window::AddPercent(unsigned int id,
                                unsigned short x,
                                unsigned short y,
                                unsigned short width,
                                unsigned short height,
                                TextureColor tc,
                                unsigned int text_color,
                                glArchivItem_Font* font,
                                const unsigned short* percentage)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlPercent(this, id, x, y, width, height, tc, text_color, font, percentage));
}

/**
 *  fügt eine ProgressBar hinzu.
 */
ctrlProgress* Window::AddProgress(unsigned int id,
                                  unsigned short x,
                                  unsigned short y,
                                  unsigned short width,
                                  unsigned short height,
                                  TextureColor tc,
                                  unsigned short button_minus,
                                  unsigned short button_plus,
                                  unsigned short maximum,
                                  const std::string& tooltip,
                                  unsigned short x_padding,
                                  unsigned short y_padding,
                                  unsigned int force_color,
                                  const std::string& button_minus_tooltip,
                                  const std::string& button_plus_tooltip)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlProgress(this, id, x, y, width, height, tc, button_minus, button_plus, maximum, x_padding, y_padding, force_color, tooltip, button_minus_tooltip, button_plus_tooltip));
}

/**
 *  fügt eine Scrollbar hinzu.
 */
ctrlScrollBar* Window::AddScrollBar(unsigned int id,
                                    unsigned short x,
                                    unsigned short y,
                                    unsigned short width,
                                    unsigned short height,
                                    unsigned short button_height,
                                    TextureColor tc,
                                    unsigned short page_size)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
        button_height = ScaleY(button_height);
    }

    return AddCtrl(id, new ctrlScrollBar(this, id, x, y, width, height, button_height, tc, page_size));
}

/**
 *  fügt ein TabCtrl hinzu.
 */
ctrlTab* Window::AddTabCtrl(unsigned int id,
                            unsigned short x,
                            unsigned short y,
                            unsigned short width)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
    }

    return AddCtrl(id, new ctrlTab(this, id, x, y, width));
}

/**
 *  fügt eine Tabelle hinzu.
 *  ... sollte eine Menge von const char*, int und SortType sein
 */
ctrlTable* Window::AddTable(unsigned int id,
                            unsigned short x,
                            unsigned short y,
                            unsigned short width,
                            unsigned short height,
                            TextureColor tc,
                            glArchivItem_Font* font,
                            unsigned int columns,
                            ...)
{
    ctrlTable* ctrl;
    va_list liste;
    va_start(liste, columns);

    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    ctrl = new ctrlTable(this, id, x, y, width, height, tc, font, columns, liste);

    va_end(liste);

    return AddCtrl(id, ctrl);
}

/**
 *  fügt einen Timer hinzu.
 */
ctrlTimer* Window::AddTimer(unsigned int id, unsigned int timeout)
{
    return AddCtrl(id, new ctrlTimer(this, id, timeout));
}

/**
 *  fügt ein TextCtrl hinzu.
 *
 *  @param[in] x      X-Koordinate des Steuerelements
 *  @param[in] y      Y-Koordinate des Steuerelements
 *  @param[in] text   Text
 *  @param[in] color  Textfarbe
 *  @param[in] format Formatierung des Textes
 *                      @p 0    - Text links ( standard )
 *                      @p glArchivItem_Font::DF_CENTER  - Text mittig
 *                      @p glArchivItem_Font::DF_RIGHT   - Text rechts
 *                      @p glArchivItem_Font::DF_TOP     - Text oben ( standard )
 *                      @p glArchivItem_Font::DF_VCENTER - Text vertikal zentriert
 *                      @p glArchivItem_Font::DF_BOTTOM  - Text unten
 *  @param[in] font   Schriftart
 */
ctrlText* Window::AddText(unsigned int id,
                          unsigned short x,
                          unsigned short y,
                          const std::string& text,
                          unsigned int color,
                          unsigned int format,
                          glArchivItem_Font* font)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
    }

    return AddCtrl(id, new ctrlText(this, id, x, y, text, color, format, font));
}

/**
 *  fügt ein vertieftes variables TextCtrl hinzu.
 */
ctrlVarDeepening* Window::AddVarDeepening(unsigned int id,
        unsigned short x,
        unsigned short y,
        unsigned short width,
        unsigned short height,
        TextureColor tc,
        const std::string& formatstr,
        glArchivItem_Font* font,
        unsigned int color,
        unsigned int parameters,
        ...)
{
    ctrlVarDeepening* ctrl;
    va_list liste;
    va_start(liste, parameters);

    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    ctrl = new ctrlVarDeepening(this, id, x, y, width, height, tc, formatstr, font, color, parameters, liste);

    va_end(liste);

    return AddCtrl(id, ctrl);
}

/**
 *  fügt ein variables TextCtrl hinzu.
 *
 *  @param[in] x          X-Koordinate des Steuerelements
 *  @param[in] y          Y-Koordinate des Steuerelements
 *  @param[in] formatstr  Der Formatstring des Steuerelements
 *  @param[in] color      Textfarbe
 *  @param[in] format     Formatierung des Textes
 *                          @p 0    - Text links ( standard )
 *                          @p glArchivItem_Font::DF_CENTER  - Text mittig
 *                          @p glArchivItem_Font::DF_RIGHT   - Text rechts
 *                          @p glArchivItem_Font::DF_TOP     - Text oben ( standard )
 *                          @p glArchivItem_Font::DF_VCENTER - Text vertikal zentriert
 *                          @p glArchivItem_Font::DF_BOTTOM  - Text unten
 *  @param[in] font       Schriftart
 *  @param[in] parameters Anzahl der nachfolgenden Parameter
 *  @param[in] ...        die variablen Parameter
 */
ctrlVarText* Window::AddVarText(unsigned int id,
                                unsigned short x,
                                unsigned short y,
                                const std::string& formatstr,
                                unsigned int color,
                                unsigned int format,
                                glArchivItem_Font* font,
                                unsigned int parameters,
                                ...)
{
    ctrlVarText* ctrl;
    va_list liste;
    va_start(liste, parameters);

    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
    }

    ctrl = new ctrlVarText(this, id, x, y, formatstr, color, format, font, parameters, liste);

    va_end(liste);

    return AddCtrl(id, ctrl);
}

ctrlPreviewMinimap* Window::AddPreviewMinimap(const unsigned id,
        unsigned short x,
        unsigned short y,
        unsigned short width,
        unsigned short height,
        glArchivItem_Map* const map)
{
    if(scale_)
    {
        x = ScaleX(x);
        y = ScaleY(y);
        width = ScaleX(width);
        height = ScaleY(height);
    }

    return AddCtrl(id, new ctrlPreviewMinimap(this, id, x, y, width, height, map));
}

/**
 *  Zeichnet einen 3D-Rahmen.
 */
void Window::Draw3D(DrawPoint drawPt,
    unsigned short width,
    unsigned short height,
    TextureColor tc,
    unsigned short type,
    bool illuminated,
    bool drawContent,
    unsigned color)
{
    if(width < 4 || height < 4 || tc == TC_INVISIBLE)
        return;

    // Position of the horizontal and vertical image border
    DrawPoint horImgBorderPos(drawPt);
    DrawPoint vertImgBorderPos(drawPt);

    if(type > 1)
    {
        // For deepened effect the img border is at bottom and right
        // else it stays top and left
        horImgBorderPos += DrawPoint(0, height - 2);
        vertImgBorderPos += DrawPoint(width - 2, 0);
    }
    // Draw img borders
    glArchivItem_Bitmap* borderImg = LOADER.GetImageN("io", 12 + tc);
    borderImg->Draw(horImgBorderPos, width, 2, 0, 0, width, 2);
    borderImg->Draw(vertImgBorderPos, 2, height, 0, 0, 2, height);

    // Draw black borders over the img borders
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_STRIP);
    // Left lower point
    DrawPoint lbPt = drawPt + DrawPoint(width, height);
    if(type <= 1)
    {
        // Bottom line with edge in left top and right line with little edge on left top
        glVertex2i(lbPt.x, drawPt.y);
        glVertex2i(lbPt.x - 2, drawPt.y + 1);
        glVertex2i(lbPt.x, lbPt.y);
        glVertex2i(lbPt.x - 2, lbPt.y - 2);
        glVertex2i(drawPt.x, lbPt.y);
        glVertex2i(drawPt.x + 1, lbPt.y - 2);
    } else
    {
        // Top line with edge on right and left line with edge on bottom
        glVertex2i(drawPt.x, lbPt.y);
        glVertex2i(drawPt.x + 2, lbPt.y - 1);
        glVertex2i(drawPt.x, drawPt.y);
        glVertex2i(drawPt.x + 2, drawPt.y + 2);
        glVertex2i(lbPt.x, drawPt.y);
        glVertex2i(lbPt.x - 1, drawPt.y + 2);
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

    DrawPoint contentPos = drawPt + DrawPoint(2, 2);
    DrawPoint contentSize(width - 4, height - 4);
    DrawPoint contentOffset(0, 0);
    if(type <= 1)
    {
        // Move the content a bit to left upper for non-deepened version
        contentOffset = DrawPoint(2, 2);
    }
    unsigned texture = (type == 1) ? tc * 2 : tc * 2 + 1;
    LOADER.GetImageN("io", texture)->Draw(contentPos, 0, 0, contentOffset.x, contentOffset.y, contentSize.x, contentSize.y, color);

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
void Window::DrawRectangle(DrawPoint drawPt, unsigned short width, unsigned short height, unsigned int color)
{
    glDisable(GL_TEXTURE_2D);

    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));

    glBegin(GL_QUADS);
    glVertex2i(drawPt.x,         drawPt.y);
    glVertex2i(drawPt.x,         drawPt.y + height);
    glVertex2i(drawPt.x + width, drawPt.y + height);
    glVertex2i(drawPt.x + width, drawPt.y);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

/**
 *  zeichnet eine Linie.
 *
 *  @param[in] x X-Koordinate
 */
void Window::DrawLine(unsigned short ax, unsigned short ay, unsigned short bx, unsigned short by, unsigned short width, unsigned int color)
{
    glDisable(GL_TEXTURE_2D);
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));

    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2i(ax, ay);
    glVertex2i(bx, by);
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
        ctrl->Move(rescale(ctrl->GetPos()));
        ctrl->Msg_ScreenResize(sr);
        ctrl->Resize(newSize);
    }
    animations_.onRescale(sr);
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
        if(IsPointInRect(mc.x, mc.y, it->second))
            return true;
    }
    return false;
}

/**
 *  skaliert einen Wert.
 */
unsigned short Window::ScaleX(unsigned short val) const
{
    return  val * VIDEODRIVER.GetScreenWidth() / 800;
}

unsigned short Window::ScaleY(unsigned short val) const
{
    return val * VIDEODRIVER.GetScreenHeight() / 600;
}
