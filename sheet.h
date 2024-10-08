#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class CellHasher 
{
public:

    size_t operator()(const Position p_) const 
    {
        return std::hash<std::string>()(p_.ToString());
    }
};

class CellComparator 
{
public:

    bool operator()(const Position& lhs_, const Position& rhs_) const 
    {
        return lhs_ == rhs_;
    }
};

class Sheet : public SheetInterface 
{
public:

    using Table = std::unordered_map<Position, std::unique_ptr<Cell>, CellHasher, CellComparator>;

    ~Sheet();

    void SetCell(Position pos, std::string text_) override;

    const CellInterface* GetCell(Position pos_) const override;
    CellInterface* GetCell(Position pos_) override;

    void ClearCell(Position pos_) override;

    Size GetPrintableSize() const override;
    const Cell* GetCellPtr(Position pos_) const;
    Cell* GetCellPtr(Position pos_);

    void PrintValues(std::ostream& output_) const override;
    void PrintTexts(std::ostream& output_) const override;

private:

    Table cells;
};