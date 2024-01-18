#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position is invalid");
    }

    Cell current_cell(*this, text);
    if (current_cell.GetType() == Cell::Type::Formula) {
        try {
            CheckCircularDependency(pos, current_cell.GetReferencedCells());
        }
        catch (const CircularDependencyException& exc) {
            throw exc;
        }
    }
    if (cells_.count(pos) == 0) {
        cells_[pos] = std::make_unique<Cell>(*this, "");
    }
    for (Position prev_parent_pos : cells_[pos]->GetReferencedCells()) {
        cells_[prev_parent_pos]->DeleteChildCell(pos);
    }
    cells_[pos]->Set(std::move(current_cell));

    /*if (cells_.count(pos)) {   
        Cell current_cell(*this, text);
        if (current_cell.GetType() == Cell::Type::Formula) {            
            try {
                CheckCircularDependency(pos, current_cell.GetReferencedCells());
            }
            catch (const CircularDependencyException& exc) {
                throw exc;
            }
        } 
        for (Position prev_parent_pos : cells_[pos]->GetReferencedCells()) {
            cells_[prev_parent_pos]->DeleteChildCell(pos);
        }
        cells_[pos]->Set(std::move(current_cell));
    }   
    if (cells_.count(pos) == 0) {
        cells_[pos] = std::make_unique<Cell>(*this, text);
    }*/

    for (Position parent_pos : cells_[pos]->GetReferencedCells()) {
        if (cells_.count(parent_pos) == 0) {
            SetCell(parent_pos, "");
        }
        cells_[parent_pos]->AddChildCell(pos);
    }
    if (cells_[pos]->IsEmpty()) {
        if (print_area_.rows == (pos.row + 1) || print_area_.cols == (pos.col + 1)) {
            print_area_ = FindNewPrintableSize();
        }
    }
    else {
        if (print_area_.rows < (pos.row + 1))
        {
            print_area_.rows = pos.row + 1;
        }
        if (print_area_.cols < (pos.col + 1))
        {
            print_area_.cols = pos.col + 1;
        }
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position is invalid");
    }
    if (cells_.count(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position is invalid");
    }
    if (cells_.count(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position is invalid");
    }
    if (cells_.count(pos) /* && !cells_.at(pos)->IsEmpty()*/) {
        //cells_[pos]->Clear();
        cells_.erase(pos);
        if (print_area_.rows == (pos.row + 1) || print_area_.cols == (pos.col + 1)) {
            print_area_ = FindNewPrintableSize();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return print_area_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int row = 0; row < print_area_.rows; ++row) {
        bool tab = false;
        for (int column = 0; column < print_area_.cols; ++column) {
            Position pos{ row, column };
            if (tab) {
                output << '\t';
            }
            tab = true;
            if (cells_.count(pos)) {
                std::visit([&output, pos](auto value) { output << value; }, cells_.at(pos)->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int row = 0; row < print_area_.rows; ++row) {
        bool tab = false;
        for (int column = 0; column < print_area_.cols; ++column) {
            Position pos{ row, column };
            if (tab) {
                output << '\t';
            }
            tab = true;
            if (cells_.count(pos)) {
                output << cells_.at(pos)->GetText();
            }
        }
        output << '\n';
    }
}

Size Sheet::FindNewPrintableSize() const {
    int max_row = -1;
    int max_col = -1;
    for (const auto& [pos, cell] : cells_) {
        if (!cell->IsEmpty()) {
            if (pos.row > max_row) {
                max_row = pos.row;
            }
            if (pos.col > max_col) {
                max_col = pos.col;
            }
        }
    }
    return { max_row + 1, max_col + 1 };
}

void Sheet::CheckCircularDependency(Position pos, const std::vector<Position>& referenced_cells) {
    if (std::binary_search(referenced_cells.begin(), referenced_cells.end(), pos)) {
        throw CircularDependencyException("Cyclic dependencies found");
    }
    if (cells_.count(pos) > 0) {
        const auto& children = cells_.at(pos)->GetChildCells();
        if (children.empty()) {
            return;
        }
        else {
            for (Position child : children) {
                CheckCircularDependency(child, referenced_cells);
            }
        }
    }
    return;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}