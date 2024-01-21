#pragma once

#include "common.h"

#include <memory>
#include <vector>

// Формула, позволяющая вычислять и обновлять арифметическое выражение.
// Поддерживаемые возможности:
// * Простые бинарные операции и числа, скобки: 1+2*3, 2.5*(2+3.5/7)
// * Значения ячеек в качестве переменных: A1+B2*C3
class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    virtual ~FormulaInterface() = default;

    // Возвращает вычисленное значение формулы для переданного листа либо ошибку.
    virtual Value Evaluate(const SheetInterface& sheet) const = 0;

    // Возвращает выражение, которое описывает формулу.
    virtual std::string GetExpression() const = 0;

    // Возвращает список ячеек, которые непосредственно задействованы в вычислении
    // формулы.
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

// Парсит переданное выражение и возвращает объект формулы.
std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);