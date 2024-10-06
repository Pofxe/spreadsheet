#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>

class Cell::Impl 
{

public:
    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const
    {
        return {}; 
    }

    virtual bool IsCacheValid() const 
    { 
        return true;
    }

    virtual void InvalidateCache() {}
};

class Cell::EmptyImpl : public Impl 
{
public:

    Value GetValue() const override 
    { 
        return "";
    }

    std::string GetText() const override 
    { 
        return "";
    }
};

class Cell::TextImpl : public Impl 
{
public:

    TextImpl(std::string text_) : text(std::move(text_)) 
    {
        if (text.empty()) 
        {
            throw std::logic_error(""); 
        }
    }

    Value GetValue() const override 
    {
        if (text[0] == ESCAPE_SIGN)
        {
            return text.substr(1);
        }
        return text;
    }

    std::string GetText() const override 
    {
        return text;
    }

private:

    std::string text;
};

class Cell::FormulaImpl : public Impl 
{
public:

    explicit FormulaImpl(std::string expression_, const SheetInterface& sheet_) : sheet(sheet_) 
    {
        if (expression_.empty() || expression_[0] != FORMULA_SIGN)
        {
            throw std::logic_error("");
        }

        formula_ptr = ParseFormula(expression_.substr(1));
    }

    Value GetValue() const override
    {
        if (!cache)
        {
            cache = formula_ptr->Evaluate(sheet);
        }

        FormulaInterface::Value value = formula_ptr->Evaluate(sheet);
        if (std::holds_alternative<double>(value))
        {
            return std::get<double>(value);
        }

        return std::get<FormulaError>(value);
    }

    std::string GetText() const override 
    {
        return FORMULA_SIGN + formula_ptr->GetExpression();
    }

    bool IsCacheValid() const override 
    {
        return cache.has_value();
    }

    void InvalidateCache() override 
    {
        cache.reset();
    }

    std::vector<Position> GetReferencedCells() const 
    {
        return formula_ptr->GetReferencedCells();
    }

private:

    std::unique_ptr<FormulaInterface> formula_ptr;
    const SheetInterface& sheet;
    mutable std::optional<FormulaInterface::Value> cache;
};

bool Cell::WouldIntroduceCircularDependency(const Impl& new_impl_) const 
{
    if (new_impl_.GetReferencedCells().empty())
    {
        return false;
    }

    std::unordered_set<const Cell*> referenced;
    for (const Position& pos : new_impl_.GetReferencedCells()) 
    {
        referenced.insert(sheet.GetCellPtr(pos));
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> to_visit;
    to_visit.push(this);

    while (!to_visit.empty()) 
    {
        const Cell* current = to_visit.top();
        to_visit.pop();
        visited.insert(current);

        if (referenced.find(current) != referenced.end())
        {
            return true;
        }

        for (const Cell* incoming : current->l_nodes) 
        {
            if (visited.find(incoming) == visited.end())
            {
                to_visit.push(incoming);
            }
        }
    }

    return false;
}

void Cell::InvalidateCacheRecursive(bool force_) 
{
    if (impl->IsCacheValid() || force_) 
    {
        impl->InvalidateCache();
        for (Cell* incoming : l_nodes) 
        {
            incoming->InvalidateCacheRecursive();
        }
    }
}

Cell::Cell(Sheet& sheet_) : impl(std::make_unique<EmptyImpl>()), sheet(sheet_) {}

Cell::~Cell() {}

void Cell::Set(std::string text_) 
{
    std::unique_ptr<Impl> impl_;

    if (text_.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text_.size() > 1 && text_[0] == FORMULA_SIGN)
    {
        impl_ = std::make_unique<FormulaImpl>(std::move(text_), sheet);
    }
    else
    {
        impl_ = std::make_unique<TextImpl>(std::move(text_));
    }

    if (WouldIntroduceCircularDependency(*impl_))
    {
        throw CircularDependencyException("");
    }
    impl = std::move(impl_);

    for (Cell* outgoing : r_nodes) 
    {
        outgoing->l_nodes.erase(this);
    }

    r_nodes.clear();

    for (const Position& pos : impl->GetReferencedCells()) 
    {
        Cell* outgoing = sheet.GetCellPtr(pos);
        if (!outgoing) 
        {
            sheet.SetCell(pos, "");
            outgoing = sheet.GetCellPtr(pos);
        }
        r_nodes.insert(outgoing);
        outgoing->l_nodes.insert(this);
    }

    InvalidateCacheRecursive(true);
}

void Cell::Clear() 
{
    impl = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const 
{
    return impl->GetValue();
}

std::string Cell::GetText() const 
{
    return impl->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const 
{
    return impl->GetReferencedCells();
}

bool Cell::IsReferenced() const 
{
    return !l_nodes.empty();
}