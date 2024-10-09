#include "FormulaAST.h"

#include "FormulaBaseListener.h"
#include "FormulaLexer.h"
#include "FormulaParser.h"

#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>

namespace ASTImpl 
{
    enum ExprPrecedence 
    {
        EP_ADD,
        EP_SUB,
        EP_MUL,
        EP_DIV,
        EP_UNARY,
        EP_ATOM,
        EP_END,
    };

    enum PrecedenceRule 
    {
        PR_NONE = 0b00,                // never needed
        PR_LEFT = 0b01,                // needed for a left child
        PR_RIGHT = 0b10,               // needed for a right child
        PR_BOTH = PR_LEFT | PR_RIGHT,  // needed for both children
    };

    
    constexpr PrecedenceRule PRECEDENCE_RULES[EP_END][EP_END] = 
    {
        /* EP_ADD */ {PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_SUB */ {PR_RIGHT, PR_RIGHT, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_MUL */ {PR_BOTH, PR_BOTH, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_DIV */ {PR_BOTH, PR_BOTH, PR_RIGHT, PR_RIGHT, PR_NONE, PR_NONE},
        /* EP_UNARY */ {PR_BOTH, PR_BOTH, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_ATOM */ {PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
    };

    class Expr 
    {
    public:

        virtual ~Expr() = default;

        virtual void Print(std::ostream& out_) const = 0;
        virtual void DoPrintFormula(std::ostream& out_, ExprPrecedence precedence_) const = 0;

        virtual double Evaluate(const SheetArgs& args_) const = 0;

        virtual ExprPrecedence GetPrecedence() const = 0;

        void PrintFormula(std::ostream& out, ExprPrecedence parent_precedence_,
            bool right_child_ = false) const 
        {
            ExprPrecedence precedence = GetPrecedence();
            PrecedenceRule mask = right_child_ ? PR_RIGHT : PR_LEFT;
            bool parens_needed = PRECEDENCE_RULES[parent_precedence_][precedence] & mask;
            if (parens_needed)
            {
                out << '(';
            }

            DoPrintFormula(out, precedence);

            if (parens_needed) 
            {
                out << ')';
            }
        }
    };

    namespace 
    {
        class BinaryOpExpr final : public Expr 
        {
        public:
            enum Type : char 
            {
                Add = '+',
                Subtract = '-',
                Multiply = '*',
                Divide = '/',
            };

            explicit BinaryOpExpr(Type type_, std::unique_ptr<Expr> lhs_, std::unique_ptr<Expr> rhs_) : type(type_), lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}

            void Print(std::ostream& out_) const override 
            {
                out_ << '(' << static_cast<char>(type) << ' ';
                lhs->Print(out_);
                out_ << ' ';
                rhs->Print(out_);
                out_ << ')';
            }

            void DoPrintFormula(std::ostream& out_, ExprPrecedence precedence_) const override 
            {
                lhs->PrintFormula(out_, precedence_);
                out_ << static_cast<char>(type);
                rhs->PrintFormula(out_, precedence_, /* right_child = */ true);
            }

            ExprPrecedence GetPrecedence() const override 
            {
                switch (type) 
                {
                case Add:
                    return EP_ADD;

                case Subtract:
                    return EP_SUB;

                case Multiply:
                    return EP_MUL;

                case Divide:
                    return EP_DIV;

                default:
                    assert(false);
                    return static_cast<ExprPrecedence>(INT_MAX);
                }
            }

            double Evaluate(const SheetArgs& args_) const override 
            {
                double lhsValue = lhs->Evaluate(args_);
                double rhsValue = rhs->Evaluate(args_);
                double result = 0.0;

                switch (type)
                {
                case Add:
                    result = lhsValue + rhsValue;
                    break;

                case Subtract:
                    result = lhsValue - rhsValue;
                    break;

                case Multiply:
                    result = lhsValue * rhsValue;
                    break;

                case Divide:
                    result = lhsValue / rhsValue;
                    break;
                }
                if (!std::isfinite(result))
                {
                    throw FormulaError{ FormulaError::Category::Div0 };
                }

                return result;
            }

        private:

            Type type;
            std::unique_ptr<Expr> lhs;
            std::unique_ptr<Expr> rhs;
        };

        class UnaryOpExpr final : public Expr 
        {
        public:
            enum Type : char 
            {
                UnaryPlus = '+',
                UnaryMinus = '-',
            };

            explicit UnaryOpExpr(Type type_, std::unique_ptr<Expr> operand_) : type(type_), operand(std::move(operand_)) {}

            void Print(std::ostream& out_) const override 
            {
                out_ << '(' << static_cast<char>(type) << ' ';
                operand->Print(out_);
                out_ << ')';
            }

            void DoPrintFormula(std::ostream& out_, ExprPrecedence precedence_) const override 
            {
                out_ << static_cast<char>(type);
                operand->PrintFormula(out_, precedence_);
            }

            ExprPrecedence GetPrecedence() const override 
            {
                return EP_UNARY;
            }

            double Evaluate(const SheetArgs& args_) const override 
            {
                if (type == UnaryMinus)
                {
                    return -1 * operand->Evaluate(args_);
                }
                else return operand->Evaluate(args_);
            }

        private:

            Type type;
            std::unique_ptr<Expr> operand;
        };

        class CellExpr final : public Expr 
        {
        public:

            explicit CellExpr(const Position* cell_) : cell(cell_) {}

            void Print(std::ostream& out_) const override 
            {
                if (!cell->IsValid()) 
                {
                    out_ << FormulaError::Category::Ref;
                }
                else 
                {
                    out_ << cell->ToString();
                }
            }

            void DoPrintFormula(std::ostream& out_, ExprPrecedence /* precedence */) const override 
            {
                Print(out_);
            }

            ExprPrecedence GetPrecedence() const override 
            {
                return EP_ATOM;
            }

            double Evaluate(const SheetArgs& args_) const override 
            {
                return args_(*cell);
            }

        private:

            const Position* cell;
        };

        class NumberExpr final : public Expr 
        {
        public:

            explicit NumberExpr(double value_) : value(value_) {}

            void Print(std::ostream& out_) const override 
            {
                out_ << value;
            }

            void DoPrintFormula(std::ostream& out_, ExprPrecedence /* precedence */) const override 
            {
                out_ << value;
            }

            ExprPrecedence GetPrecedence() const override 
            {
                return EP_ATOM;
            }

            double Evaluate(const SheetArgs& /*args*/) const override 
            {
                return value;
            }

        private:

            double value;
        };

        class ParseASTListener final : public FormulaBaseListener 
        {
        public:

            std::unique_ptr<Expr> MoveRoot() 
            {
                assert(args.size() == 1);
                std::unique_ptr<Expr> root = std::move(args.front());
                args.clear();

                return root;
            }

            std::forward_list<Position> MoveCells() 
            {
                return std::move(cells);
            }

            void exitUnaryOp(FormulaParser::UnaryOpContext* ctx_) override 
            {
                assert(args.size() >= 1);

                std::unique_ptr<Expr> operand = std::move(args.back());

                UnaryOpExpr::Type type;
                if (ctx_->SUB()) 
                {
                    type = UnaryOpExpr::UnaryMinus;
                }
                else 
                {
                    assert(ctx_->ADD() != nullptr);
                    type = UnaryOpExpr::UnaryPlus;
                }

                auto node = std::make_unique<UnaryOpExpr>(type, std::move(operand));
                args.back() = std::move(node);
            }

            void exitLiteral(FormulaParser::LiteralContext* ctx_) override 
            {
                double value = 0;
                std::string valueStr = ctx_->NUMBER()->getSymbol()->getText();
                std::istringstream in(valueStr);
                in >> value;

                if (!in) 
                {
                    throw ParsingError("Invalid number: " + valueStr);
                }

                auto node = std::make_unique<NumberExpr>(value);
                args.push_back(std::move(node));
            }

            void exitCell(FormulaParser::CellContext* ctx_) override 
            {
                std::string value_str = ctx_->CELL()->getSymbol()->getText();
                Position value = Position::FromString(value_str);

                if (!value.IsValid())
                {
                    throw FormulaException("Invalid position: " + value_str);
                }

                cells.push_front(value);
                auto node = std::make_unique<CellExpr>(&cells.front());
                args.push_back(std::move(node));
            }

            void exitBinaryOp(FormulaParser::BinaryOpContext* ctx_) override 
            {
                assert(args.size() >= 2);

                std::unique_ptr<Expr> rhs = std::move(args.back());
                args.pop_back();

                std::unique_ptr<Expr> lhs = std::move(args.back());

                BinaryOpExpr::Type type;
                if (ctx_->ADD()) 
                {
                    type = BinaryOpExpr::Add;
                }
                else if (ctx_->SUB()) 
                {
                    type = BinaryOpExpr::Subtract;
                }
                else if (ctx_->MUL()) 
                {
                    type = BinaryOpExpr::Multiply;
                }
                else 
                {
                    assert(ctx_->DIV() != nullptr);
                    type = BinaryOpExpr::Divide;
                }

                auto node = std::make_unique<BinaryOpExpr>(type, std::move(lhs), std::move(rhs));
                args.back() = std::move(node);
            }

            void visitErrorNode(antlr4::tree::ErrorNode* node_) override 
            {
                throw ParsingError("Error when parsing: " + node_->getSymbol()->getText());
            }

        private:

            std::vector<std::unique_ptr<Expr>> args;
            std::forward_list<Position> cells;
        };

        class BailErrorListener : public antlr4::BaseErrorListener 
        {
        public:
            void syntaxError(antlr4::Recognizer* /* recognizer */, antlr4::Token* /* offendingSymbol */, size_t /* line */, size_t /* charPositionInLine */, const std::string& msg_, std::exception_ptr /* e */) override 
            {
                throw ParsingError("Error when lexing: " + msg_);
            }
        };

    }  // namespace
}  // namespace ASTImpl

FormulaAST ParseFormulaAST(std::istream& in_) 
{
    using namespace antlr4;

    ANTLRInputStream input(in_);

    FormulaLexer lexer(&input);
    ASTImpl::BailErrorListener error_listener;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&error_listener);

    CommonTokenStream tokens(&lexer);

    FormulaParser parser(&tokens);
    auto error_handler = std::make_shared<BailErrorStrategy>();
    parser.setErrorHandler(error_handler);
    parser.removeErrorListeners();

    tree::ParseTree* tree = parser.main();
    ASTImpl::ParseASTListener listener;
    tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

    return FormulaAST(listener.MoveRoot(), listener.MoveCells());
}

FormulaAST ParseFormulaAST(const std::string& in_str_)
{
    std::istringstream in(in_str_);
    return ParseFormulaAST(in);
}

void FormulaAST::PrintCells(std::ostream& out_) const 
{
    for (Position cell : cells) 
    {
        out_ << cell.ToString() << ' ';
    }
}

void FormulaAST::Print(std::ostream& out_) const 
{
    root_expr->Print(out_);
}

void FormulaAST::PrintFormula(std::ostream& out_) const 
{
    root_expr->PrintFormula(out_, ASTImpl::EP_ATOM);
}

double FormulaAST::Execute(const SheetArgs& args_) const 
{
    return root_expr->Evaluate(args_);
}

FormulaAST::FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr_, std::forward_list<Position> cells_) : root_expr(std::move(root_expr_)), cells(std::move(cells_))
{
    cells.sort();
}

FormulaAST::~FormulaAST() = default;