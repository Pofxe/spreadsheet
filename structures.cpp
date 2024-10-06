#include "common.h"

#include <cctype>
#include <sstream>
#include <algorithm>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = { -1, -1 };

bool Position::operator==(const Position rhs_) const 
{
    return row == rhs_.row && col == rhs_.col;
}

bool Position::operator<(const Position rhs_) const 
{
    return std::tie(row, col) < std::tie(rhs_.row, rhs_.col);
}

bool Position::IsValid() const 
{
    return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
}

std::string Position::ToString() const 
{
    if (!IsValid()) 
    {
        return "";
    }

    std::string result;
    result.reserve(MAX_POSITION_LENGTH);
    int c = col;

    while (c >= 0) 
    {
        result.insert(result.begin(), 'A' + c % LETTERS);
        c = c / LETTERS - 1;
    }

    result += std::to_string(row + 1);

    return result;
}

Position Position::FromString(std::string_view str_) 
{
    std::string_view::const_iterator it = std::find_if(str_.begin(), str_.end(), [](const char c) 
        {
        return !(std::isalpha(c) && std::isupper(c));
        });

    std::string_view letters = str_.substr(0, it - str_.begin());
    std::string_view digits = str_.substr(it - str_.begin());

    if (letters.empty() || digits.empty())
    {
        return Position::NONE;
    }
    if (letters.size() > MAX_POS_LETTER_COUNT) 
    {
        return Position::NONE;
    }

    if (!std::isdigit(digits[0])) 
    {
        return Position::NONE;
    }

    int row = 0;
    std::istringstream row_in{ std::string{digits} };

    if (!(row_in >> row) || !row_in.eof()) 
    {
        return Position::NONE;
    }

    int col = 0;
    for (char ch : letters) 
    {
        col *= LETTERS;
        col += ch - 'A' + 1;
    }

    return { row - 1, col - 1 };
}

bool Size::operator==(Size rhs_) const 
{
    return cols == rhs_.cols && rows == rhs_.rows;
}