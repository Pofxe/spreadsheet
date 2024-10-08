#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category_) : category(category_) {}

FormulaError::Category FormulaError::GetCategory() const
{
    return category;
}

bool FormulaError::operator==(FormulaError rhs) const 
{
    return category == rhs.category;
}

std::string_view FormulaError::ToString() const 
{
    switch (category) 
    {
    case Category::Ref:
        return "#REF!";

    case Category::Value:
        return "#VALUE!";

    case Category::Div0:
        return "#ARITHM!";
    }
    return "";
}

std::ostream& operator<<(std::ostream& output_, FormulaError fe_) 
{
    return output_ << fe_.ToString();
}

namespace 
{
    class Formula : public FormulaInterface
    {
    public:

        explicit Formula(const std::string expression_) : ast(ParseFormulaAST(expression_)) {}

        Value Evaluate(const SheetInterface& sheet_) const override 
        {
            const std::function<double(Position)> args = [&sheet_](const Position p)->double 
                {
                    if (!p.IsValid())
                    {
                        throw FormulaError(FormulaError::Category::Ref);
                    }

                const CellInterface* cell = sheet_.GetCell(p);
                if (!cell)
                {
                    return 0;
                }
                if (std::holds_alternative<double>(cell->GetValue()))
                {
                    return std::get<double>(cell->GetValue());
                }
                if (std::holds_alternative<std::string>(cell->GetValue())) 
                {
                    std::string value = std::get<std::string>(cell->GetValue());
                    double result = 0;
                    if (!value.empty()) 
                    {
                        std::istringstream in(value);
                        if (!(in >> result) || !in.eof())
                        {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                    return result;
                }
                throw FormulaError(std::get<FormulaError>(cell->GetValue()));
                };

            try 
            {
                return ast.Execute(args);
            }
            catch (FormulaError& e) 
            {
                return e;
            }
        }

        std::vector<Position> GetReferencedCells() const override 
        {
            std::vector<Position> cells;
            for (Position cell : ast.GetCells()) 
            {
                if (cell.IsValid())
                {
                    cells.push_back(cell);
                }
            }
            cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
            return cells;
        }

        std::string GetExpression() const override 
        {
            std::ostringstream out;
            ast.PrintFormula(out);
            return out.str();
        }

    private:

        const FormulaAST ast;
    };

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression_) 
{
    try
    {
        return std::make_unique<Formula>(std::move(expression_));
    }
    catch (...)
    {
        throw FormulaException("");
    }
}