#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>

namespace lqr_sim {

template <std::size_t Rows, std::size_t Cols>
class Matrix {
 public:
  Matrix() = default;

  Matrix(std::initializer_list<double> values) {
    if (values.size() != data_.size()) {
      throw std::invalid_argument("Matrix initializer has the wrong size");
    }
    std::copy(values.begin(), values.end(), data_.begin());
  }

  [[nodiscard]] double& operator()(std::size_t row, std::size_t col) {
    return data_.at(row * Cols + col);
  }

  [[nodiscard]] const double& operator()(std::size_t row,
                                         std::size_t col) const {
    return data_.at(row * Cols + col);
  }

  [[nodiscard]] static Matrix Zero() { return {}; }

  [[nodiscard]] static Matrix Identity() {
    static_assert(Rows == Cols, "Identity matrix must be square");
    Matrix result;
    for (std::size_t index = 0; index < Rows; ++index) {
      result(index, index) = 1.0;
    }
    return result;
  }

 private:
  std::array<double, Rows * Cols> data_{};
};

template <std::size_t Rows, std::size_t Cols>
[[nodiscard]] Matrix<Rows, Cols> operator+(const Matrix<Rows, Cols>& lhs,
                                            const Matrix<Rows, Cols>& rhs) {
  Matrix<Rows, Cols> result;
  for (std::size_t row = 0; row < Rows; ++row) {
    for (std::size_t col = 0; col < Cols; ++col) {
      result(row, col) = lhs(row, col) + rhs(row, col);
    }
  }
  return result;
}

template <std::size_t Rows, std::size_t Cols>
[[nodiscard]] Matrix<Rows, Cols> operator-(const Matrix<Rows, Cols>& lhs,
                                            const Matrix<Rows, Cols>& rhs) {
  Matrix<Rows, Cols> result;
  for (std::size_t row = 0; row < Rows; ++row) {
    for (std::size_t col = 0; col < Cols; ++col) {
      result(row, col) = lhs(row, col) - rhs(row, col);
    }
  }
  return result;
}

template <std::size_t Rows, std::size_t Inner, std::size_t Cols>
[[nodiscard]] Matrix<Rows, Cols> operator*(const Matrix<Rows, Inner>& lhs,
                                            const Matrix<Inner, Cols>& rhs) {
  Matrix<Rows, Cols> result;
  for (std::size_t row = 0; row < Rows; ++row) {
    for (std::size_t col = 0; col < Cols; ++col) {
      for (std::size_t inner = 0; inner < Inner; ++inner) {
        result(row, col) += lhs(row, inner) * rhs(inner, col);
      }
    }
  }
  return result;
}

template <std::size_t Rows, std::size_t Cols>
[[nodiscard]] Matrix<Cols, Rows> Transpose(const Matrix<Rows, Cols>& input) {
  Matrix<Cols, Rows> result;
  for (std::size_t row = 0; row < Rows; ++row) {
    for (std::size_t col = 0; col < Cols; ++col) {
      result(col, row) = input(row, col);
    }
  }
  return result;
}

template <std::size_t Size>
[[nodiscard]] Matrix<Size, Size> Inverse(const Matrix<Size, Size>& input) {
  Matrix<Size, Size> left = input;
  Matrix<Size, Size> right = Matrix<Size, Size>::Identity();

  for (std::size_t pivot_col = 0; pivot_col < Size; ++pivot_col) {
    std::size_t pivot_row = pivot_col;
    for (std::size_t row = pivot_col + 1; row < Size; ++row) {
      if (std::abs(left(row, pivot_col)) >
          std::abs(left(pivot_row, pivot_col))) {
        pivot_row = row;
      }
    }
    if (std::abs(left(pivot_row, pivot_col)) < 1e-12) {
      throw std::runtime_error("Matrix is singular");
    }
    for (std::size_t col = 0; col < Size; ++col) {
      std::swap(left(pivot_col, col), left(pivot_row, col));
      std::swap(right(pivot_col, col), right(pivot_row, col));
    }

    const double pivot = left(pivot_col, pivot_col);
    for (std::size_t col = 0; col < Size; ++col) {
      left(pivot_col, col) /= pivot;
      right(pivot_col, col) /= pivot;
    }

    for (std::size_t row = 0; row < Size; ++row) {
      if (row == pivot_col) {
        continue;
      }
      const double factor = left(row, pivot_col);
      for (std::size_t col = 0; col < Size; ++col) {
        left(row, col) -= factor * left(pivot_col, col);
        right(row, col) -= factor * right(pivot_col, col);
      }
    }
  }
  return right;
}

template <std::size_t Rows, std::size_t Cols>
[[nodiscard]] double MaxAbsDifference(const Matrix<Rows, Cols>& lhs,
                                      const Matrix<Rows, Cols>& rhs) {
  double maximum = 0.0;
  for (std::size_t row = 0; row < Rows; ++row) {
    for (std::size_t col = 0; col < Cols; ++col) {
      maximum = std::max(maximum, std::abs(lhs(row, col) - rhs(row, col)));
    }
  }
  return maximum;
}

}  // namespace lqr_sim
