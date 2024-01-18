#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <functional>

using namespace std::literals;

FormulaError::FormulaError(Category category)
    :category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
    case Category::Ref:
        return "#REF!"sv;
    case Category::Arithmetic:
        return "#ARITHM!"sv;
    case Category::Value:
        return "#VALUE!"sv;
    }
    return {};
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:

    explicit Formula(std::string expression)
    : ast_(ParseFormulaAST(expression)) {}

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            std::function<const CellInterface*(Position)> functor = [&sheet](Position pos) { return sheet.GetCell(pos); };
            return ast_.Execute(functor);
        }
        catch (const FormulaError& exc) {
            return exc;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream output;
        ast_.PrintFormula(output);
        return output.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result(ast_.GetCells().begin(), ast_.GetCells().end());
        auto it = std::unique(result.begin(), result.end());
        result.erase(it, result.end());
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("Formula parsing error");
    }
}