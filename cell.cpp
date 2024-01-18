#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue(const SheetInterface& sheet) const = 0;
    virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    EmptyImpl() = default;

    CellInterface::Value GetValue(const SheetInterface& sheet) const override {
        return {};
    }
    std::string GetText() const override {
        return {};
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl(std::string text) : text_(std::move(text)) {};

    CellInterface::Value GetValue(const SheetInterface& sheet) const override {
        if (text_.front() == ESCAPE_SIGN) {
            return std::string(text_, 1, std::string::npos);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl(std::string text)
        :formula_(ParseFormula(text)) {}

    CellInterface::Value GetValue(const SheetInterface& sheet) const override {
        FormulaInterface::Value result = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        }
        else return std::get<FormulaError>(result);
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }

    std::string GetText() const override {
        return '=' + formula_->GetExpression();
    }


private:
    std::unique_ptr<FormulaInterface> formula_;
};

// Реализуйте следующие методы
Cell::Cell(const SheetInterface& sheet, std::string text)
    :sheet_(sheet) 
{
    Set(text);
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        type_ = Type::Empty;
    } else
    if (text.front() == FORMULA_SIGN && (text.size() > 1)) {
        impl_ = std::make_unique<FormulaImpl>(text.substr(1, std::string::npos));
        type_ = Type::Formula;       
    }
    else {
        impl_ = std::make_unique<TextImpl>(text);
        type_ = Type::Text;
    }

}

void Cell::Set(Cell&& cell) {
    impl_ = std::move(cell.impl_);
    type_ = std::move(cell.type_);
    ResetCash();
}

void Cell::Clear() {
    if (!IsEmpty()) {
        impl_ = std::make_unique<EmptyImpl>();
        type_ = Type::Empty;
    }    
}

bool Cell::IsEmpty() const {
    return type_ == Type::Empty;
}

Cell::Type Cell::GetType() const {
    return type_;
}

CellInterface::Value Cell::GetValue() const {
    if (type_ == Type::Formula) {
        if (!cash_) {
            CellInterface::Value value = impl_->GetValue(sheet_);
            if (std::holds_alternative<double>(value)) {
                cash_ = std::get<double>(value);
            }
            else return std::get<FormulaError>(value);
        }
        return cash_.value();
    }
    return impl_->GetValue(sheet_);
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

void Cell::ResetCash() const {
    if (!cash_) {
        return;
    }
    cash_.reset();
    if (child_cells_.empty()) {
        return;
    }
    for (Position child : child_cells_) {
        reinterpret_cast<const Cell*>(sheet_.GetCell(child))->ResetCash();
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    if (type_ == Type::Formula) {
        return reinterpret_cast<Cell::FormulaImpl*>(impl_.get())->GetReferencedCells();
    }
    return {};
}


void Cell::AddChildCell(Position cell) {
    child_cells_.insert(cell);
}

void Cell::DeleteChildCell(Position cell) {
    child_cells_.erase(cell);
}

const std::unordered_set<Position, PositionHasher>& Cell::GetChildCells() const {
    return child_cells_;
}

