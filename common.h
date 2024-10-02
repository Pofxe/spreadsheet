#pragma once

#include <stdexcept>

class FormulaError : public std::runtime_error 
{
public:

    using std::runtime_error::runtime_error;
};

std::ostream& operator<<(std::ostream& output_, FormulaError fe_);


class FormulaException : public std::runtime_error 
{
public:

    using std::runtime_error::runtime_error;
};