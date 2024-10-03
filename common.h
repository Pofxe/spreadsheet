#pragma once

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

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

inline constexpr char FORMULA_SIGN = '=';
inline constexpr char ESCAPE_SIGN = '\'';


class CellInterface 
{
public:

    using Value = std::variant<std::string, double, FormulaError>;

    virtual ~CellInterface() = default;

    virtual void Set(std::string text) = 0;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
};