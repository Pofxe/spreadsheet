#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

std::ostream& operator<<(std::ostream& output_, FormulaError /*fe*/)
{
    return output_ << "#DIV/0!";
}

namespace 
{
    class Formula : public FormulaInterface 
    {
    public:

        explicit Formula(std::string expression_) : ast(ParseFormulaAST(expression_)) {}

        Value Evaluate() const override 
        {
            Value result;
            try 
            {
                result = ast.Execute();
            }
            catch (const FormulaError& evaluate_error) 
            {
                result = evaluate_error;
            }
            return result;
        }

        std::string GetExpression() const override 
        {
            std::ostringstream out;
            ast.PrintFormula(out);

            return out.str();
        }

    private:

        FormulaAST ast;
    };

}//end namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression_) 
{
    try 
    {
        return std::make_unique<Formula>(expression_);
    }
    catch (...) 
    {
        throw FormulaException("Parsing formula from expression was failure");
    }
}