#pragma once

template <typename T> class DataSize {
private:
  T m_value;  // NOLINT

public:
  enum class Unit {
    B = 1,
    KB = 1024,
    MB = 1024 * 1024,
    GB = 1024 * 1024 * 1024,
    TB = 1024 * 1024 * 1024 * 1024,
    PB = 1024 * 1024 * 1024 * 1024 * 1024,
    EB = 1024 * 1024 * 1024 * 1024 * 1024 * 1024,
    ZB = 1024 * 1024 * 1024 * 1024 * 1024 * 1024 * 1024,
    YB = 1024 * 1024 * 1024 * 1024 * 1024 * 1024 * 1024 * 1024
  };

  DataSize(T value) : m_value(value) {}

  T get_value() const { return m_value; }

  DataSize<T> operator+(const DataSize<T> &rhs) const { return DataSize<T>(m_value + rhs.m_value); }

  DataSize<T> operator-(const DataSize<T> &rhs) const { return DataSize<T>(m_value - rhs.m_value); }

  DataSize<T> operator*(const DataSize<T> &rhs) const { return DataSize<T>(m_value * rhs.m_value); }

  DataSize<T> operator/(const DataSize<T> &rhs) const { return DataSize<T>(m_value / rhs.m_value); }

  DataSize<T> operator%(const DataSize<T> &rhs) const { return DataSize<T>(m_value % rhs.m_value); }

  DataSize<T> operator+=(const DataSize<T> &rhs) {
    m_value += rhs.m_value;
    return *this;
  }

  DataSize<T> operator-=(const DataSize<T> &rhs) {
    m_value -= rhs.m_value;
    return *this;
  }

  DataSize<T> operator*=(const DataSize<T> &rhs) {
    m_value *= rhs.m_value;
    return *this;
  }

  DataSize<T> operator/=(const DataSize<T> &rhs) {
    m_value /= rhs.m_value;
    return *this;
  }

  DataSize<T> operator%=(const DataSize<T> &rhs) {
    m_value %= rhs.m_value;
    return *this;
  }

  bool operator==(const DataSize<T> &rhs) const { return m_value == rhs.m_value; }

  bool operator!=(const DataSize<T> &rhs) const { return m_value != rhs.m_value; }

  bool operator<(const DataSize<T> &rhs) const { return m_value < rhs.m_value; }

  bool operator>(const DataSize<T> &rhs) const { return m_value > rhs.m_value; }

  bool operator<=(const DataSize<T> &rhs) const { return m_value <= rhs.m_value; }

  bool operator>=(const DataSize<T> &rhs) const { return m_value >= rhs.m_value; }

  template <typename U> DataSize<U> convert_to(DataSize<U>::Unit unit) const {
    return DataSize<U>(static_cast<U>(m_value) / static_cast<U>(unit));
  }
};