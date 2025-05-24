// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTable.h"
#include "CollisionDetection.h"
#include "ctrlButton.h"
#include "ctrlScrollBar.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "helpers/mathFuncs.h"
#include "ogl/glFont.h"
#include "s25util/StringConversion.h"
#include "s25util/strAlgos.h"
#include <algorithm>
#include <numeric>
#include <sstream>

/// Return <0 for a < b; >0 for a>b and 0 for a==b
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
            s25util::ClassicImbuedStream<std::istringstream> ss_a(a);
            s25util::ClassicImbuedStream<std::istringstream> ss_b(b);
            char x;
            int x_a, y_a, x_b, y_b;
            ss_a >> x_a >> x >> y_a;
            ss_b >> x_b >> x >> y_b;
            RTTR_Assert(ss_a);
            RTTR_Assert(ss_b);
            const int result = x_a * y_a - x_b * y_b;
            // In case of same number of nodes sort by first value
            return (result == 0) ? x_a - x_b : result;
        }
        break;
        case SRT::Number:
        {
            s25util::ClassicImbuedStream<std::istringstream> ss_a(a);
            s25util::ClassicImbuedStream<std::istringstream> ss_b(b);
            int num_a, num_b;
            ss_a >> num_a;
            ss_b >> num_b;
            RTTR_Assert(ss_a);
            RTTR_Assert(ss_b);
            return num_a - num_b;
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
                return y_a - y_b;

            if(m_a != m_b)
                return m_a - m_b;

            if(d_a != d_b)
                return d_a - d_b;

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
                return h_a - h_b;
            return min_a - min_b;
        }
        case SRT::Time:
        {
            // Sort by time with format h:mm:ss or mm:ss
            s25util::ClassicImbuedStream<std::istringstream> ss_a(a);
            s25util::ClassicImbuedStream<std::istringstream> ss_b(b);

            int seconds_a = 0;
            int seconds_b = 0;
            char c;

            // "h:mm:ss" or "mm:ss"
            int tmp;
            while(ss_a >> tmp)
            {
                seconds_a *= 60;
                seconds_a += tmp;
                ss_a >> c;
            }
            while(ss_b >> tmp)
            {
                seconds_b *= 60;
                seconds_b += tmp;
                ss_b >> c;
            }

            return seconds_a - seconds_b;
        }
        break;
    }
    return 0;
}

ctrlTable::ctrlTable(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                     const glFont* font, Columns columns)
    : Window(parent, id, pos, elMax(size, Extent(20, 30))), tc(tc), font(font), columns_(std::move(columns)),
      selection_(-1), sortColumn_(-1), sortDir_(TableSortDir::Ascending)
{
    // We use unsigned short when handling the column count
    if(columns_.size() > std::numeric_limits<unsigned short>::max())
        throw std::range_error("Maximum amount of columns exceeded");

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
    // Make header buttons keep their size
    for(unsigned i = 1; i <= columns_.size(); ++i)
        GetCtrl<ctrlButton>(i)->SetHeight(header_height);

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

    SetSelection(boost::none);
    sortColumn_ = -1;
    sortDir_ = TableSortDir::Ascending;
}

/**
 *  setzt die Auswahl.
 *
 *  @param[in] selection Der Auswahlindex
 */
void ctrlTable::SetSelection(const boost::optional<unsigned>& selection)
{
    if(!selection)
        selection_ = boost::none;
    else if(*selection >= rows_.size())
        return;
    else
    {
        selection_ = selection;
        // Scroll into view
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        if(*selection_ < scrollbar->GetScrollPos())
            scrollbar->SetScrollPos(*selection_);
        else if(*selection_ >= static_cast<unsigned>(scrollbar->GetScrollPos() + scrollbar->GetPageSize()))
            scrollbar->SetScrollPos(*selection_ - scrollbar->GetPageSize() + 1);
    }

    if(GetParent())
        GetParent()->Msg_TableSelectItem(GetID(), selection_);
}

void ctrlTable::AddRow(std::vector<std::string> row)
{
    // We use unsigned short when handling the row count
    if(rows_.size() == std::numeric_limits<unsigned short>::max())
        throw std::range_error("Maximum amount of rows exceeded");
    if(row.size() > GetNumColumns())
        throw std::logic_error("Invalid number of columns for added row");
    for(unsigned i = row.size(); i < GetNumColumns(); ++i)
    {
        row.push_back("");
    }

    rows_.emplace_back(Row{std::move(row)});
    GetCtrl<ctrlScrollBar>(0)->SetRange(GetNumRows());
}

void ctrlTable::RemoveRow(unsigned rowIdx)
{
    if(rowIdx >= rows_.size())
        return;
    rows_.erase(rows_.begin() + rowIdx);
    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(rows_.size()));
    if(selection_ && *selection_ >= rows_.size())
    {
        if(rows_.empty())
            selection_.reset();
        else
            selection_ = rows_.size() - 1u;
    }
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

    return rows_[row].columns[column];
}

/**
 *  sortiert die Zeilen.
 *
 *  @param[in] column    Die Spalte nach der sortiert werden soll.
 *  @param[in] ascending Die Richtung in die sortiert werden soll
 *                         @p true  - A-Z,
 *                         @p false - Z-A
 */
void ctrlTable::SortRows(unsigned column, const TableSortDir sortDir)
{
    sortDir_ = sortDir;
    sortColumn_ = column;

    if(sortColumn_ >= GetNumColumns())
    {
        sortColumn_ = -1;
        return;
    }

    const TableSortType sortType = columns_[column].sortType;
    const auto compareFunc = [sortType, sortDir, column](const Row& lhs, const Row& rhs) {
        const std::string a = s25util::toLower(lhs.columns[column]);
        const std::string b = s25util::toLower(rhs.columns[column]);
        const int cmpResult = Compare(a, b, sortType);
        return (sortDir == TableSortDir::Ascending) ? cmpResult < 0 : cmpResult > 0;
    };
    std::sort(rows_.begin(), rows_.end(), compareFunc);
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

    const auto lines = static_cast<unsigned>(line_count > rows_.size() ? rows_.size() : line_count);
    const auto* scroll = GetCtrl<ctrlScrollBar>(0);
    DrawPoint curPos = GetDrawPos() + DrawPoint(2, 2 + header_height);
    for(unsigned i = 0; i < lines; ++i)
    {
        const unsigned curRow = i + scroll->GetScrollPos();
        RTTR_Assert(curRow < GetNumRows());
        const bool isSelected = selection_ == curRow;
        if(isSelected)
        {
            // durchsichtig schwarze Markierung malen
            DrawRectangle(Rect(curPos, GetSize().x - 4 - (scroll->IsVisible() ? 24 : 0), font->getHeight()),
                          0x80000000);
        }

        DrawPoint colPos = curPos;
        for(unsigned c = 0; c < columns_.size(); ++c)
        {
            if(columns_[c].width == 0)
                continue;

            const auto* bt = GetCtrl<ctrlButton>(c + 1);
            font->Draw(colPos, rows_[curRow].columns[c], FontStyle{}, (isSelected ? 0xFFFFAA00 : COLOR_YELLOW),
                       bt->GetSize().x, "");
            colPos.x += bt->GetSize().x;
        }
        curPos.y += font->getHeight();
    }
}

void ctrlTable::Msg_ButtonClick(const unsigned ctrl_id)
{
    RTTR_Assert(ctrl_id > 0);
    const int column = ctrl_id - 1;
    SortRows(column, column == GetSortColumn() && GetSortDirection() == TableSortDir::Ascending ?
                       TableSortDir::Descending :
                       TableSortDir::Ascending);
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

boost::optional<unsigned> ctrlTable::GetSelectionFromMouse(const MouseCoords& mc) const
{
    const int visibleItem = (mc.pos.y - GetContentDrawArea().top) / font->getHeight();
    if(visibleItem < 0)
        return boost::none;
    const unsigned itemIdx = static_cast<unsigned>(visibleItem) + GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
    if(itemIdx >= GetNumRows())
        return boost::none;
    return itemIdx;
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
            const auto oldSelection = GetSelection();
            SetSelection(GetSelectionFromMouse(mc));
            if(selection_ && oldSelection == selection_)
            {
                GetParent()->Msg_TableChooseItem(this->GetID(), *selection_);
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
    const unsigned availableSize =
      std::max<int>(0, GetSize().x - (scrollbar->IsVisible() ? scrollbar->GetSize().x : 0));
    const double sizeFactor = static_cast<double>(availableSize) / static_cast<double>(sumWidth);
    DrawPoint btPos(0, 0);
    for(unsigned i = 0; i < columns_.size(); ++i)
    {
        auto* button = GetCtrl<ctrlButton>(i + 1);
        button->SetWidth(helpers::iround<unsigned>(columns_[i].width * sizeFactor));
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
        case KeyType::Up:
            if(selection_.value_or(0u) > 0u)
                SetSelection(*selection_ - 1);
            return true;
        case KeyType::Down: SetSelection(selection_.value_or(0u) + 1u); return true;
    }
}
