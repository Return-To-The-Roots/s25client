// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTable.h"
#include "CollisionDetection.h"
#include "ctrlButton.h"
#include "ctrlScrollBar.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "helpers/containerUtils.h"
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
    line_count = (getSize().y - header_height - 2) / font->getHeight();

    // Scrollbar hinzufügen
    AddScrollBar(0, DrawPoint(getSize().x - 20, 0), Extent(20, getSize().y), 20, tc, line_count);

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
    const bool heightIncreased = newSize.y > getSize().y;
    Window::Resize(newSize);
    // Make header buttons keep their size
    for(unsigned i = 1; i <= columns_.size(); ++i)
        getCtrl<ctrlButton>(i)->setHeight(header_height);

    auto* scrollbar = getCtrl<ctrlScrollBar>(0);

    // changed height

    scrollbar->Resize(Extent(20, newSize.y));
    scrollbar->setPos(DrawPoint(newSize.x - scrollbar->getSize().x, 0));

    line_count = (newSize.y - header_height - 2) / font->getHeight();
    scrollbar->setPageSize(line_count);

    // If the size was enlarged we have to check that we don't try to
    // display more lines than present
    if(heightIncreased)
        while(rows_.size() - scrollbar->getScrollPos() < line_count && scrollbar->getScrollPos() > 0)
            scrollbar->setScrollPos(scrollbar->getScrollPos() - 1);

    // changed width

    ResetButtonWidths();
    if(scrollbar->isVisible())
        msgScrollShow(0, true);
}

/**
 *  löscht alle Items.
 */
void ctrlTable::DeleteAllItems()
{
    rows_.clear();

    getCtrl<ctrlScrollBar>(0)->setRange(0);

    setSelection(std::nullopt);
    sortColumn_ = -1;
    sortDir_ = TableSortDir::Ascending;
}

/**
 *  setzt die Auswahl.
 *
 *  @param[in] selection Der Auswahlindex
 */
void ctrlTable::setSelection(const std::optional<unsigned>& selection)
{
    if(!selection)
        selection_ = std::nullopt;
    else if(*selection >= rows_.size())
        return;
    else
    {
        selection_ = selection;
        // Scroll into view
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        if(*selection_ < scrollbar->getScrollPos())
            scrollbar->setScrollPos(*selection_);
        else if(*selection_ >= static_cast<unsigned>(scrollbar->getScrollPos() + scrollbar->getPageSize()))
            scrollbar->setScrollPos(*selection_ - scrollbar->getPageSize() + 1);
    }

    if(getParent())
        getParent()->msgTableSelectItem(getID(), selection_);
}

void ctrlTable::AddRow(std::vector<std::string> row)
{
    // We use unsigned short when handling the row count
    if(rows_.size() == std::numeric_limits<unsigned short>::max())
        throw std::range_error("Maximum amount of rows exceeded");
    if(row.size() > getNumColumns())
        throw std::logic_error("Invalid number of columns for added row");
    for(unsigned i = row.size(); i < getNumColumns(); ++i)
    {
        row.push_back("");
    }

    rows_.emplace_back(Row{std::move(row)});
    getCtrl<ctrlScrollBar>(0)->setRange(getNumRows());
}

void ctrlTable::RemoveRow(unsigned rowIdx)
{
    if(rowIdx >= rows_.size())
        return;
    rows_.erase(rows_.begin() + rowIdx);
    getCtrl<ctrlScrollBar>(0)->setRange(static_cast<unsigned short>(rows_.size()));
    if(selection_ && *selection_ >= rows_.size())
    {
        if(rows_.empty())
            selection_.reset();
        else
            selection_ = rows_.size() - 1u;
    }
    setSelection(selection_);
}

/**
 *  liefert den Wert eines Feldes.
 *
 *  @param[in] row    Die Zeile
 *  @param[in] column Die Spalte
 *
 *  @return Text in Feld <@p column,@p row>
 */
const std::string& ctrlTable::getItemText(unsigned short row, unsigned short column) const
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

    if(sortColumn_ >= getNumColumns())
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
    helpers::sort(rows_, compareFunc);
}

/**
 *  Zeichenmethode
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlTable::Draw_()
{
    Draw3D(Rect(getDrawPos(), getSize()), tc, false);

    Window::Draw_();

    const auto lines = static_cast<unsigned>(line_count > rows_.size() ? rows_.size() : line_count);
    const auto* scroll = getCtrl<ctrlScrollBar>(0);
    DrawPoint curPos = getDrawPos() + DrawPoint(2, 2 + header_height);
    for(unsigned i = 0; i < lines; ++i)
    {
        const unsigned curRow = i + scroll->getScrollPos();
        RTTR_Assert(curRow < getNumRows());
        const bool isSelected = selection_ == curRow;
        if(isSelected)
        {
            // durchsichtig schwarze Markierung malen
            DrawRectangle(Rect(curPos, getSize().x - 4 - (scroll->isVisible() ? 24 : 0), font->getHeight()),
                          0x80000000);
        }

        DrawPoint colPos = curPos;
        for(unsigned c = 0; c < columns_.size(); ++c)
        {
            if(columns_[c].width == 0)
                continue;

            const auto* bt = getCtrl<ctrlButton>(c + 1);
            font->Draw(colPos, rows_[curRow].columns[c], FontStyle{}, (isSelected ? 0xFFFFAA00 : COLOR_YELLOW),
                       bt->getSize().x, "");
            colPos.x += bt->getSize().x;
        }
        curPos.y += font->getHeight();
    }
}

void ctrlTable::msgButtonClick(const unsigned ctrl_id)
{
    RTTR_Assert(ctrl_id > 0);
    const int column = ctrl_id - 1;
    SortRows(column, column == getSortColumn() && getSortDirection() == TableSortDir::Ascending ?
                       TableSortDir::Descending :
                       TableSortDir::Ascending);
    setSelection(selection_);
}

Rect ctrlTable::getContentDrawArea() const
{
    DrawPoint orig = getDrawPos();
    orig.y += header_height;
    Extent size = getSize() - Extent(20, header_height);
    return Rect(orig, size);
}

Rect ctrlTable::getFullDrawArea() const
{
    DrawPoint orig = getDrawPos() + DrawPoint(2, 2);
    Extent size = getSize() - Extent(2, 4);
    return Rect(orig, size);
}

bool ctrlTable::msgLeftDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.pos, getContentDrawArea()))
    {
        setSelection(getSelectionFromMouse(mc));
        if(getParent())
            getParent()->msgTableLeftButton(this->getID(), selection_);

        return true;
    } else
        return RelayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlTable::msgRightDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.pos, getContentDrawArea()))
    {
        setSelection(getSelectionFromMouse(mc));
        if(getParent())
            getParent()->msgTableRightButton(this->getID(), selection_);

        return true;
    } else
        return RelayMouseMessage(&Window::msgRightDown, mc);
}

std::optional<unsigned> ctrlTable::getSelectionFromMouse(const MouseCoords& mc) const
{
    const int visibleItem = (mc.pos.y - getContentDrawArea().top) / font->getHeight();
    if(visibleItem < 0)
        return std::nullopt;
    const unsigned itemIdx = static_cast<unsigned>(visibleItem) + getCtrl<ctrlScrollBar>(0)->getScrollPos();
    if(itemIdx >= getNumRows())
        return std::nullopt;
    return itemIdx;
}

bool ctrlTable::msgWheelUp(const MouseCoords& mc)
{
    // If mouse in list or scrollbar
    if(IsPointInRect(mc.pos, getFullDrawArea()))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-1);
        return true;
    } else
        return false;
}

bool ctrlTable::msgWheelDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.pos, getFullDrawArea()))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+1);
        return true;
    } else
        return false;
}

bool ctrlTable::msgLeftUp(const MouseCoords& mc)
{
    if(IsPointInRect(mc.pos, getContentDrawArea()))
    {
        if(mc.dbl_click && getParent())
        {
            const auto oldSelection = getSelection();
            setSelection(getSelectionFromMouse(mc));
            if(selection_ && oldSelection == selection_)
            {
                getParent()->msgTableChooseItem(this->getID(), *selection_);
                return true;
            }
        }
    }
    return RelayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlTable::msgMouseMove(const MouseCoords& mc)
{
    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::msgMouseMove, mc);
}

void ctrlTable::msgScrollShow(const unsigned /*ctrl_id*/, const bool /*visible*/)
{
    ResetButtonWidths();
}

void ctrlTable::ResetButtonWidths()
{
    auto addColumnWidth = [](unsigned cur, const Column& c) { return cur + c.width; };
    const unsigned sumWidth = std::max(1u, std::accumulate(columns_.begin(), columns_.end(), 0u, addColumnWidth));
    const auto* scrollbar = getCtrl<ctrlScrollBar>(0);
    const unsigned availableSize =
      std::max<int>(0, getSize().x - (scrollbar->isVisible() ? scrollbar->getSize().x : 0));
    const double sizeFactor = static_cast<double>(availableSize) / static_cast<double>(sumWidth);
    DrawPoint btPos(0, 0);
    for(unsigned i = 0; i < columns_.size(); ++i)
    {
        auto* button = getCtrl<ctrlButton>(i + 1);
        button->setWidth(helpers::iround<unsigned>(columns_[i].width * sizeFactor));
        button->setPos(btPos);
        btPos.x += button->getSize().x;
    }

    // Adjust last column size to cater for rounding error
    for(unsigned i = columns_.size(); i > 0; --i)
    {
        if(columns_[i - 1].width)
        {
            auto* bt = getCtrl<ctrlButton>(i);
            bt->setWidth(std::max(0, static_cast<int>(availableSize) - bt->getPos().x));
            break;
        }
    }
}

bool ctrlTable::msgKeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default: return false;
        case KeyType::Up:
            if(selection_.value_or(0u) > 0u)
                setSelection(*selection_ - 1);
            return true;
        case KeyType::Down: setSelection(selection_.value_or(0u) + 1u); return true;
    }
}
