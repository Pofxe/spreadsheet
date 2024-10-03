#include "FormulaAST.h"
#include "common.h"
#include "formula.h"
#include "test_runner_p.h"

using Value = FormulaInterface::Value;

Value ExecuteFormula(const std::string& expression_)
{
    return ParseFormula(expression_)->Evaluate();
}

int main() 
{
    
}