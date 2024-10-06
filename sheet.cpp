#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos_, std::string text_) 
{
    if (!pos_.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    const auto& cell = cells.find(pos_);

    if (cell == cells.end())
    {
        cells.emplace(pos_, std::make_unique<Cell>(*this));
    }
    cells.at(pos_)->Set(std::move(text_));
}

const CellInterface* Sheet::GetCell(Position pos_) const 
{
    return GetCellPtr(pos_);
}

CellInterface* Sheet::GetCell(Position pos_) 
{
    return GetCellPtr(pos_);
}

void Sheet::ClearCell(Position pos_) 
{
    if (!pos_.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    const auto& cell = cells.find(pos_);
    if (cell != cells.end() && cell->second != nullptr) 
    {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) 
        {
            cell->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const 
{
    Size result{ 0, 0 };

    for (auto it = cells.begin(); it != cells.end(); ++it) 
    {
        if (it->second != nullptr) 
        {
            const int col = it->first.col;
            const int row = it->first.row;
            result.rows = std::max(result.rows, row + 1);
            result.cols = std::max(result.cols, col + 1);
        }
    }

    return { result.rows, result.cols };
}

void Sheet::PrintValues(std::ostream& output_) const 
{
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) 
    {
        for (int col = 0; col < size.cols; ++col) 
        {
            if (col > 0)
            {
                output_ << "\t";
            }

            const auto& it = cells.find({ row, col });

            if (it != cells.end() && it->second != nullptr && !it->second->GetText().empty())
            {
                std::visit([&](const auto value) { output_ << value; }, it->second->GetValue());
            }
        }
        output_ << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output_) const 
{
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) 
    {
        for (int col = 0; col < size.cols; ++col) 
        {
            if (col > 0)
            {
                output_ << "\t";
            }

            const auto& it = cells.find({ row, col });

            if (it != cells.end() && it->second != nullptr && !it->second->GetText().empty()) 
            {
                output_ << it->second->GetText();
            }
        }
        output_ << "\n";
    }
}

const Cell* Sheet::GetCellPtr(Position pos_) const 
{
    if (!pos_.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    const auto cell = cells.find(pos_);
    if (cell == cells.end()) 
    {
        return nullptr;
    }

    return cells.at(pos_).get();
}

Cell* Sheet::GetCellPtr(Position pos_) 
{
    return const_cast<Cell*>( static_cast<const Sheet&>(*this).GetCellPtr(pos_));
}

std::unique_ptr<SheetInterface> CreateSheet() 
{
    return std::make_unique<Sheet>();
}