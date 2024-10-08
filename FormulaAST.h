#pragma once

#include "FormulaLexer.h"
#include "common.h"

#include <forward_list>
#include <functional>
#include <stdexcept>

namespace ASTImpl 
{
    class Expr;
}

class ParsingError : public std::runtime_error 
{
    using std::runtime_error::runtime_error;
};

using SheetArgs = std::function<double(Position)>;

class FormulaAST 
{
public:

    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr_, std::forward_list<Position> cells_);
    FormulaAST(FormulaAST&&) = default;

    FormulaAST& operator=(FormulaAST&&) = default;

    ~FormulaAST();

    double Execute(const SheetArgs& args_) const;

    void PrintCells(std::ostream& out_) const;
    void Print(std::ostream& out_) const;
    void PrintFormula(std::ostream& out_) const;

    std::forward_list<Position>& GetCells() 
    {
        return cells;
    }

    const std::forward_list<Position>& GetCells() const 
    {
        return cells;
    }

private:

    std::unique_ptr<ASTImpl::Expr> root_expr;
    std::forward_list<Position> cells;
};

FormulaAST ParseFormulaAST(std::istream& in_);
FormulaAST ParseFormulaAST(const std::string& in_str_);