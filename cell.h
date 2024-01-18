#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

struct PositionHasher {
    size_t operator() (Position pos) const {
        return pos.row * 37 + pos.col * 37 * 37;
    }
};

class Cell : public CellInterface {
public:

    enum class Type {
        Empty,
        Text,
        Formula
    };

    explicit Cell(const SheetInterface& sheet, std::string text = "");
    ~Cell();

    void Set(std::string text);
    void Set(Cell&& cell);
    void Clear();

    bool IsEmpty() const;

    Type GetType() const;

    Value GetValue() const override;
    std::string GetText() const override;

    void ResetCash() const;

    std::vector<Position> GetReferencedCells() const override;

    void AddChildCell(Position);
    void DeleteChildCell(Position);
    const std::unordered_set<Position, PositionHasher>& GetChildCells() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    Type type_;
    const SheetInterface& sheet_;
    std::unordered_set<Position, PositionHasher> child_cells_;
    mutable std::optional<double> cash_;
};