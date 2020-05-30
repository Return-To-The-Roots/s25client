// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "ctrlTable.h"
#include "CollisionDetection.h"
#include "ctrlButton.h"
#include "ctrlScrollBar.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"
#include "s25util/StringConversion.h"
#include "s25util/strAlgos.h"
#include <cmath>
#include <numeric>
#include <sstream>

static int Compare(const std::string& a, const std::string& b, ctrlTable::SortType sortType)
{
    using SRT = ctrlTable::SortType;
    switch(sortType)
    {
        case SRT::Default:
        case SRT::String: return a.compare(b); break;
        case SRT::MapSize:
        {
            // Nach Mapgrößen-String sortieren: ZahlxZahl
            std::istringstream ss_a(a);
            std::istringstream ss_b(b);
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
        case SRT::Number:
        {
            std::istringstream ss_a(a);
            std::istringstream ss_b(b);
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
        case SRT::Date:
        {
            // Nach Datum im Format dd.mm.yyyy - hh:mm sortieren
            s25util::ClassicImbuedStream<std::istringstream> ss_a(a);
            s25util::ClassicImbuedStream<std::istringstream> ss_b(b);
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

ctrlTable::ctrlTable(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                     std::vector<Column> columns)
    : Window(parent, id, pos, elMax(size, Extent(20, 30))), tc(tc), font(font), columns_(std::move(columns)), selection_(-1),
      sort_column(-1), sort_direction(true)
{
    header_height = font->getHeight() + 10;
    line_count = (GetSize().y - header_height - 2) / font->getHeight();

    // Scrollbar hinzufügen
    AddScrollBar(0, DrawPoint(GetSize().x - 20, 0), Extent(20, GetSize().y), 20, tc, line_count);

    for(unsigned i = 0; i < columns_.size(); ++i)
    {
        AddTextButton(i + 1, DrawPoint(0, 0), Extent(0, header_height), tc, columns_[i].title, font);
    }

    ResetButtonWidths();
}

/**
 *  Größe verändern
 */
void ctrlTable::Resize(const Extent& newSize)
{
    const bool heightIncreased = newSize.y > GetSize().y;
    Window::Resize(newSize);

    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // changed height

    scrollbar->Resize(Extent(20, newSize.y));
    scrollbar->SetPos(DrawPoint(newSize.x - scrollbar->GetSize().x, 0));

    line_count = (newSize.y - header_height - 2) / font->getHeight();
    scrollbar->SetPageSize(line_count);

    // If the size was enlarged we have to check that we don't try to
    // display more lines than present
    if(heightIncreased)
        while(rows_.size() - scrollbar->GetScrollPos() < line_count && scrollbar->GetScrollPos() > 0)
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
    rows_.clear();

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
    else if(static_cast<unsigned>(selection) >= rows_.size())
        return;
    else
    {
        selection_ = selection;
        // Scroll into view
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        if(selection_ < scrollbar->GetScrollPos())
            scrollbar->SetScrollPos(selection_);
        else if(selection_ >= scrollbar->GetScrollPos() + scrollbar->GetPageSize())
            scrollbar->SetScrollPos(selection_ - scrollbar->GetPageSize() + 1);
    }

    if(GetParent())
        GetParent()->Msg_TableSelectItem(GetID(), selection_);
}

void ctrlTable::AddRow(std::vector<std::string> row)
{
    if(row.size() > GetNumColumns())
        throw std::logic_error("Invalid number of columns for added row");
    for(unsigned i = row.size(); i < GetNumColumns(); ++i)
    {
        row.push_back("");
    }

    rows_.emplace_back(Row{std::move(row)});
    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(rows_.size()));
}

void ctrlTable::RemoveRow(unsigned rowIdx)
{
    if(rowIdx >= rows_.size())
        return;
    rows_.erase(rows_.begin() + rowIdx);
    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(rows_.size()));
    if(selection_ >= 0 && static_cast<unsigned>(selection_) >= rows_.size())
        selection_ = static_cast<int>(rows_.size()) - 1;
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
    if(row >= rows_.size() || column >= columns_.size())
        return empty;

    return rows_.at(row).columns.at(column);
}

/**
 *  sortiert die Zeilen.
 *
 *  @param[in] column    Die Spalte nach der sortiert werden soll.
 *  @param[in] direction Die Richtung in die sortiert werden soll
 *                         @p nullptr  - Wechsel,
 *                         @p true  - A-Z,
 *                         @p false - Z-A
 */
void ctrlTable::SortRows(int column, const bool* direction)
{
    if(columns_.empty())
        return;
    if(rows_.empty())
        return;
    if(column >= static_cast<int>(columns_.size()))
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

    // On which value of compare (-1,0,1) to swap
    int sortCompareValue = sort_direction ? 1 : -1;
    bool done;
    do
    {
        done = true;
        for(unsigned r = 0; r < rows_.size() - 1; ++r)
        {
            // Compare lowercase values
            const std::string a = s25util::toLower(rows_[r].columns[sort_column]);
            const std::string b = s25util::toLower(rows_[r + 1].columns[sort_column]);

            if(Compare(a, b, columns_[column].sortType) == sortCompareValue)
            {
                using std::swap;
                swap(rows_[r], rows_[r + 1]);
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

    Window::Draw_();

    const auto lines = static_cast<int>(line_count > rows_.size() ? rows_.size() : line_count);
    const auto* scroll = GetCtrl<ctrlScrollBar>(0);
    DrawPoint curPos = GetDrawPos() + DrawPoint(2, 2 + header_height);
    for(int i = 0; i < lines; ++i)
    {
        const int curRow = i + scroll->GetScrollPos();
        RTTR_Assert(curRow >= 0 && curRow < GetNumRows());
        const bool isSelected = selection_ == curRow;
        if(isSelected)
        {
            // durchsichtig schwarze Markierung malen
            DrawRectangle(Rect(curPos, GetSize().x - 4 - (scroll->IsVisible() ? 24 : 0), font->getHeight()), 0x80000000);
        }

        DrawPoint colPos = curPos;
        for(unsigned c = 0; c < columns_.size(); ++c)
        {
            if(columns_[c].width == 0)
                continue;

            const auto* bt = GetCtrl<ctrlButton>(c + 1);
            font->Draw(colPos, rows_[curRow].columns[c], FontStyle{}, (isSelected ? 0xFFFFAA00 : COLOR_YELLOW), bt->GetSize().x, "");
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
    return (mc.pos.y - GetContentDrawArea().top) / font->getHeight() + GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
}

bool ctrlTable::Msg_WheelUp(const MouseCoords& mc)
{
    // If mouse in list or scrollbar
    if(IsPointInRect(mc.GetPos(), GetFullDrawArea()))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-1);
        return true;
    } else
        return false;
}

bool ctrlTable::Msg_WheelDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), GetFullDrawArea()))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
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

void ctrlTable::Msg_ScrollShow(const unsigned /*ctrl_id*/, const bool /*visible*/)
{
    ResetButtonWidths();
}

void ctrlTable::ResetButtonWidths()
{
    auto addColumnWidth = [](unsigned cur, const Column& c) { return cur + c.width; };
    const unsigned sumWidth = std::max(1u, std::accumulate(columns_.begin(), columns_.end(), 0u, addColumnWidth));
    const auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
    const unsigned availableSize = std::max<int>(0, GetSize().x - (scrollbar->IsVisible() ? scrollbar->GetSize().x : 0));
    const double sizeFactor = static_cast<double>(availableSize) / static_cast<double>(sumWidth);
    DrawPoint btPos(0, 0);
    for(unsigned i = 0; i < columns_.size(); ++i)
    {
        auto* button = GetCtrl<ctrlButton>(i + 1);
        button->SetWidth(std::lround(columns_[i].width * sizeFactor));
        button->SetPos(btPos);
        btPos.x += button->GetSize().x;
    }

    // Adjust last column size to cater for rounding error
    for(unsigned i = columns_.size(); i > 0; --i)
    {
        if(columns_[i - 1].width)
        {
            auto* bt = GetCtrl<ctrlButton>(i);
            bt->SetWidth(std::max(0, static_cast<int>(availableSize) - bt->GetPos().x));
            break;
        }
    }
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
