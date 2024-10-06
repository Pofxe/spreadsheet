#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface 
{
public:

    Cell(Sheet& sheet_);
    ~Cell();

    void Set(std::string text_);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:

    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    bool WouldIntroduceCircularDependency(const Impl& new_impl_) const;
    void InvalidateCacheRecursive(bool force_ = false);

    std::unique_ptr<Impl> impl;
    Sheet& sheet;
    std::unordered_set<Cell*> l_nodes;
    std::unordered_set<Cell*> r_nodes;
};