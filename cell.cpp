#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell() : impl(std::make_unique<EmptyImpl>()) {}
Cell::~Cell() = default;

void Cell::Set(std::string text_) 
{
    if (text_.empty()) 
    {
        impl = std::make_unique<EmptyImpl>();
        return;

    }
    else if (text_.size() >= 2 && text_.at(0) == FORMULA_SIGN) 
    {
        impl = std::move(std::make_unique<FormulaImpl>(std::move(text_)));
        return;
    }
    else 
    {
        impl = std::move(std::make_unique<TextImpl>(std::move(text_)));
        return;
    }
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

Cell::Value Cell::EmptyImpl::GetValue() const 
{
    return ""; 
}

std::string Cell::EmptyImpl::GetText() const 
{ 
    return "";
}

Cell::TextImpl::TextImpl(std::string text_) : text(std::move(text_)) {}

Cell::Value Cell::TextImpl::GetValue() const 
{
    if (text.empty()) 
    {
        throw std::logic_error("it is empty impl, not text");
    }
    else if (text.at(0) == ESCAPE_SIGN) 
    {
        return text.substr(1);
    }
    else {
        return text;
    }
}

std::string Cell::TextImpl::GetText() const 
{ 
    return text; 
}

Cell::FormulaImpl::FormulaImpl(std::string text_) : formula_ptr(ParseFormula(text_.substr(1))) {}

Cell::Value Cell::FormulaImpl::GetValue() const 
{
    FormulaInterface::Value helper = formula_ptr->Evaluate();

    if (std::holds_alternative<double>(helper)) 
    {
        return std::get<double>(helper);
    }
    else 
    {
        return std::get<FormulaError>(helper);
    }
}

std::string Cell::FormulaImpl::GetText() const 
{
    return FORMULA_SIGN + formula_ptr->GetExpression();
}