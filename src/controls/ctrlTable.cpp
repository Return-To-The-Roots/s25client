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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ctrlTable.h"
#include "CollisionDetection.h"
#include "ctrlButton.h"
#include "ctrlScrollBar.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "ogl/glArchivItem_Font.h"
#include "libutil/StringConversion.h"
#include <cstdarg>
#include <sstream>

ctrlTable::ctrlTable(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font,
                     unsigned short column_count, va_list liste)
    : Window(parent, id, pos, elMax(size, Extent(20, 30))), tc(tc), font(font), selection_(-1), sort_column(-1), sort_direction(true)
{
    header_height = font->getHeight() + 10;
    line_count = (GetSize().y - header_height - 2) / font->getHeight();

    // Scrollbar hinzufügen
    AddScrollBar(0, DrawPoint(GetSize().x - 20, 0), Extent(20, GetSize().y), 20, tc, line_count);

    if(column_count > 0)
    {
        for(unsigned short i = 0; i < column_count; ++i)
        {
            COLUMN c;

            const char* title = va_arg(liste, const char*);
            if(title)
                c.title = title;

            c.width = (unsigned short)va_arg(liste, int);
            c.sortType = (SortType)va_arg(liste, int);

            // Button für die Spalte hinzufügen
            AddTextButton(i + 1, DrawPoint(0, 0), Extent(0, header_height), tc, c.title, font);

            columns.push_back(c);
        }

        ResetButtonWidths();
    }
}

ctrlTable::~ctrlTable()
{
    DeleteAllItems();
    columns.clear();
}

/**
 *  Größe verändern
 */
void ctrlTable::Resize(const Extent& newSize)
{
    const bool heightIncreased = newSize.y > GetSize().y;
    Window::Resize(newSize);

    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // changed height

    scrollbar->SetPos(DrawPoint(newSize.x - 20, 0));
    scrollbar->Resize(Extent(20, newSize.y));

    line_count = (newSize.y - header_height - 2) / font->getHeight();
    scrollbar->SetPageSize(line_count);

    // If the size was enlarged we have to check that we don't try to
    // display more lines than present
    if(heightIncreased)
        while(rows.size() - scrollbar->GetScrollPos() < line_count && scrollbar->GetScrollPos() > 0)
            scrollbar->SetScrollPos(scrollbar->GetScrollPos() - 1);

    // changed width

    ResetButtonWidths();
    if(scrollbar->IsVisible())
        Msg_ScrollShow(0, true);
}

/**
 *  löscht alle Items.
 */
void ctrlTable::DeleteAllItems()
{
    rows.clear();

    GetCtrl<ctrlScrollBar>(0)->SetRange(0);

    SetSelection(-1);
    sort_column = -1;
    sort_direction = true;
}

/**
 *  setzt die Auswahl.
 *
 *  @param[in] selection Der Auswahlindex
 */
void ctrlTable::SetSelection(int selection)
{
    if(selection < 0)
        selection_ = -1;
    else if(static_cast<unsigned>(selection) >= rows.size())
        return;
    else
    {
        selection_ = selection;
        // Scroll into view
        ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
        if(selection_ < scrollbar->GetScrollPos())
            scrollbar->SetScrollPos(selection_);
        else if(selection_ >= scrollbar->GetScrollPos() + scrollbar->GetPageSize())
            scrollbar->SetScrollPos(selection_ - scrollbar->GetPageSize() + 1);
    }

    if(GetParent())
        GetParent()->Msg_TableSelectItem(GetID(), selection_);
}

/**
 *  fügt eine Zeile hinzu.
 *
 *  @param[in] alwaysnull Immer 0, wird nur für Liste gebraucht
 *  @param[in] ...        Die Werte für die Spalten.
 */
void ctrlTable::AddRow(unsigned alwaysnull, ...)
{
    va_list liste;
    va_start(liste, alwaysnull);

    ROW r;
    for(unsigned short i = 0; i < columns.size(); ++i)
    {
        const char* text = va_arg(liste, const char*);

        if(text)
            r.columns.push_back(text);
        else
            r.columns.push_back("");
    }
    va_end(liste);

    rows.push_back(r);
    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(rows.size()));
}

void ctrlTable::RemoveRow(unsigned rowIdx)
{
    if(rowIdx >= rows.size())
        return;
    rows.erase(rows.begin() + rowIdx);
    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(rows.size()));
    if(selection_ >= 0 && static_cast<unsigned>(selection_) >= rows.size())
        selection_ = static_cast<int>(rows.size()) - 1;
    SetSelection(selection_);
}

/**
 *  liefert den Wert eines Feldes.
 *
 *  @param[in] row    Die Zeile
 *  @param[in] column Die Spalte
 *
 *  @return Text in Feld <@p column,@p row>
 */
const std::string& ctrlTable::GetItemText(unsigned short row, unsigned short column) const
{
    static std::string empty;
    if(row >= rows.size() || column >= columns.size())
        return empty;

    return rows.at(row).columns.at(column);
}

/**
 *  sortiert die Zeilen.
 *
 *  @param[in] column    Die Spalte nach der sortiert werden soll.
 *  @param[in] direction Die Richtung in die sortiert werden soll
 *                         @p NULL  - Wechsel,
 *                         @p true  - A-Z,
 *                         @p false - Z-A
 */
void ctrlTable::SortRows(int column, bool* direction)
{
    if(columns.empty())
        return;
    if(rows.empty())
        return;
    if(column >= static_cast<int>(columns.size()))
        column = 0;

    if(direction)
        sort_direction = *direction;
    else if(sort_column == column)
        sort_direction = !sort_direction;
    else
        sort_direction = true;
    sort_column = column;

    if(sort_column < 0 || sort_column >= GetNumColumns())
        return;

    bool done;
    do
    {
        done = true;
        for(unsigned r = 0; r < rows.size() - 1; ++r)
        {
            std::string a = rows[r].columns[sort_column];
            std::string b = rows[r + 1].columns[sort_column];

            // in kleinbuchstaben vergleichen
            std::transform(a.begin(), a.end(), a.begin(), tolower);
            std::transform(b.begin(), b.end(), b.begin(), tolower);

            if((sort_direction && Compare(a, b, columns[column].sortType) > 0)
               || (!sort_direction && Compare(a, b, columns[column].sortType) < 0))
            {
                using std::swap;
                swap(rows[r], rows[r + 1]);
                done = false;
            }
        }
    } while(!done);
}

/**
 *  Zeichenmethode
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlTable::Draw_()
{
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    DrawControls();

    int lines = static_cast<int>(line_count > rows.size() ? rows.size() : line_count);
    ctrlScrollBar* scroll = GetCtrl<ctrlScrollBar>(0);
    DrawPoint curPos = GetDrawPos() + DrawPoint(2, 2 + header_height);
    for(int i = 0; i < lines; ++i)
    {
        const int curRow = i + scroll->GetScrollPos();
        RTTR_Assert(curRow >= 0 && curRow < GetNumRows());
        bool isSelected = selection_ == curRow;
        if(isSelected)
        {
            // durchsichtig schwarze Markierung malen
            DrawRectangle(Rect(curPos, GetSize().x - 4 - (scroll->IsVisible() ? 24 : 0), font->getHeight()), 0x80000000);
        }

        DrawPoint colPos = curPos;
        for(unsigned short c = 0; c < columns.size(); ++c)
        {
            if(columns[c].width == 0)
                continue;

            ctrlButton* bt = GetCtrl<ctrlButton>(c + 1);
            font->Draw(colPos, rows[curRow].columns[c], 0, (isSelected ? 0xFFFFAA00 : COLOR_YELLOW), 0, bt->GetSize().x, "");
            colPos.x += bt->GetSize().x;
        }
        curPos.y += font->getHeight();
    }
}

void ctrlTable::Msg_ButtonClick(const unsigned ctrl_id)
{
    SortRows(ctrl_id - 1);
    SetSelection(selection_);
}

Rect ctrlTable::GetContentDrawArea() const
{
    DrawPoint orig = GetDrawPos();
    orig.y += header_height;
    Extent size = GetSize() - Extent(20, header_height);
    return Rect(orig, size);
}

Rect ctrlTable::GetFullDrawArea() const
{
    DrawPoint orig = GetDrawPos() + DrawPoint(2, 2);
    Extent size = GetSize() - Extent(2, 4);
    return Rect(orig, size);
}

bool ctrlTable::Msg_LeftDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), GetContentDrawArea()))
    {
        SetSelection(GetSelectionFromMouse(mc));
        if(GetParent())
            GetParent()->Msg_TableLeftButton(this->GetID(), selection_);

        return true;
    } else
        return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlTable::Msg_RightDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), GetContentDrawArea()))
    {
        SetSelection(GetSelectionFromMouse(mc));
        if(GetParent())
            GetParent()->Msg_TableRightButton(this->GetID(), selection_);

        return true;
    } else
        return RelayMouseMessage(&Window::Msg_RightDown, mc);
}

int ctrlTable::GetSelectionFromMouse(const MouseCoords& mc)
{
    return (mc.y - GetContentDrawArea().top) / font->getHeight() + GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
}

bool ctrlTable::Msg_WheelUp(const MouseCoords& mc)
{
    // If mouse in list or scrollbar
    if(IsPointInRect(mc.GetPos(), GetFullDrawArea()))
    {
        ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-1);
        return true;
    } else
        return false;
}

bool ctrlTable::Msg_WheelDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), GetFullDrawArea()))
    {
        ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+1);
        return true;
    } else
        return false;
}

bool ctrlTable::Msg_LeftUp(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), GetContentDrawArea()))
    {
        if(mc.dbl_click && GetParent())
        {
            int selection = GetSelectionFromMouse(mc);
            SetSelection(selection);
            if(selection_ >= 0 && selection == selection_)
            {
                GetParent()->Msg_TableChooseItem(this->GetID(), selection_);
                return true;
            }
        }
    }
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlTable::Msg_MouseMove(const MouseCoords& mc)
{
    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

void ctrlTable::Msg_ScrollShow(const unsigned /*ctrl_id*/, const bool visible)
{
    if(visible)
    {
        /// Scrollbar wird angezeigt
        // Breite der letzten Spalte entsprechend anpassen, wenn plötzlich ne Scrolleiste sich hier noch reindrängelt
        // Aufteilen dieser Breite auf die einzelnen Spalten
        unsigned x_col_minus = unsigned(20 / columns.size());
        // Rest, der nicht aufgeteilt wurde
        unsigned rest = unsigned(20 % columns.size());

        for(unsigned i = 0; i < columns.size(); ++i)
        {
            ctrlButton* bt = GetCtrl<ctrlButton>(i + 1);
            if(bt->GetSize().x > x_col_minus) //-V807

                bt->SetWidth(bt->GetSize().x - x_col_minus);
            else
                rest += x_col_minus;
        }

        // Rest einfach von letzter passender Spalte abziehen
        for(unsigned i = 0; i < columns.size(); ++i)
        {
            ctrlButton* bt = GetCtrl<ctrlButton>(unsigned(columns.size()) - i - 1 + 1);
            if(bt->GetSize().x > rest)
            {
                bt->SetWidth(bt->GetSize().x - rest);
                break;
            }
        }

        // X-Position der Buttons neu berechnen
        DrawPoint btPos(0, 0);
        for(unsigned i = 0; i < columns.size(); ++i)
        {
            GetCtrl<ctrlButton>(i + 1)->SetPos(btPos);
            btPos.x += GetCtrl<ctrlButton>(i + 1)->GetSize().x;
        }
    } else
    {
        // Scrollbar wird nicht mehr angezeigt --> Breite und Position wieder zurücksetzen
        ResetButtonWidths();
    }
}

/// Setzt die Breite und Position der Buttons ohne Scrolleiste
void ctrlTable::ResetButtonWidths()
{
    // Scrollbar wird nicht mehr angezeigt --> Breite und Position wieder zurücksetzen
    DrawPoint btPos(0, 0);
    for(unsigned i = 0; i < columns.size(); ++i)
    {
        GetCtrl<ctrlButton>(i + 1)->SetWidth(columns[i].width * GetSize().x / 1000);
        GetCtrl<ctrlButton>(i + 1)->SetPos(btPos);
        btPos.x += GetCtrl<ctrlButton>(i + 1)->GetSize().x;
    }

    // Rest auf dem letzten aufschlagen
    for(unsigned i = 0; i < columns.size(); ++i)
    {
        if(columns.at(columns.size() - i - 1).width)
        {
            ctrlButton* bt = GetCtrl<ctrlButton>(unsigned(columns.size()) - i - 1 + 1);
            bt->SetWidth(bt->GetSize().x + (GetSize().x - btPos.x));
            break;
        }
    }
}

/// Verschiedene Sortiermöglichkeiten
int ctrlTable::Compare(const std::string& a, const std::string& b, SortType sortType)
{
    switch(sortType)
    {
        case SRT_DEFAULT:
        case SRT_STRING:
            return a.compare(b);
            break;
        // Nach Mapgrößen-String sortieren: ZahlxZahl
        case SRT_MAPSIZE:
        {
            std::stringstream ss_a(a);
            std::stringstream ss_b(b);
            char x;
            int x_a, y_a, x_b, y_b;
            ss_a >> x_a >> x >> y_a;
            ss_b >> x_b >> x >> y_b;
            RTTR_Assert(ss_a);
            RTTR_Assert(ss_b);
            if(x_a * y_a == x_b * y_b)
                return 0;
            else
                return (x_a * y_a < x_b * y_b) ? -1 : 1;
        }
        break;
        // Nach Zahl sortieren
        case SRT_NUMBER:
        {
            std::stringstream ss_a(a);
            std::stringstream ss_b(b);
            int num_a, num_b;
            ss_a >> num_a;
            ss_b >> num_b;
            RTTR_Assert(ss_a);
            RTTR_Assert(ss_b);
            if(num_a == num_b)
                return 0;
            else
                return (num_a < num_b) ? -1 : 1;
        }
        break;
        // Nach Datum im Format dd.mm.yyyy - hh:mm sortieren
        case SRT_DATE:
        {
            s25util::ClassicImbuedStream<std::stringstream> ss_a(a);
            s25util::ClassicImbuedStream<std::stringstream> ss_b(b);
            int d_a, d_b, m_a, m_b, y_a, y_b;
            char c;

            // "dd.mm.yyyy"
            ss_a >> d_a >> c >> m_a >> c >> y_a;
            ss_b >> d_b >> c >> m_b >> c >> y_b;

            RTTR_Assert(ss_a);
            RTTR_Assert(ss_b);
            if(y_a != y_b)
                return (y_a < y_b) ? -1 : 1;

            if(m_a != m_b)
                return (m_a < m_b) ? -1 : 1;

            if(d_a != d_b)
                return (d_a < d_b) ? -1 : 1;

            // " - "
            ss_a >> c;
            ss_b >> c;

            int h_a, h_b, min_a, min_b;

            // "hh:mm"
            ss_a >> h_a >> c >> min_a;
            ss_b >> h_b >> c >> min_b;

            RTTR_Assert(ss_a);
            RTTR_Assert(ss_b);
            if(h_a != h_b)
                return (h_a < h_b) ? -1 : 1;
            if(min_a != min_b)
                return (min_a < min_b) ? -1 : 1;

            return 0;
        }
        break;
    }
    return 0;
}

bool ctrlTable::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default: return false;
        case KT_UP:
            if(selection_ > 0)
                SetSelection(selection_ - 1);
            return true;
        case KT_DOWN: SetSelection(selection_ + 1); return true;
    }
}
