#pragma once
#include <cstdint>
#include <iostream>

enum class DataUnit : uint64_t {
  B = 1,
  KB = 1024,
  MB = 1048576,
  GB = 1073741824,
  TB = 1099511627776,
  PB = 1125899906842624,
};

template <DataUnit T> class DataSize {
private:
  uint64_t m_value;  // NOLINT
  bool m_allow_rounding;

public:
  DataSize(uint64_t value, bool allow_rounding = false)
      : m_value(value), m_allow_rounding(allow_rounding) {}

  uint64_t get_value() const { return m_value; }

  // Arithmetic operators handling different units
  template <DataUnit U> DataSize<T> operator+(const DataSize<U> &rhs) const {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    return DataSize<T>(m_value + converted_rhs);
  }

  template <DataUnit U> DataSize<T> operator-(const DataSize<U> &rhs) const {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    return DataSize<T>(m_value - converted_rhs);
  }
  template <DataUnit U> DataSize<T> operator*(const DataSize<U> &rhs) const {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    return DataSize<T>(m_value * converted_rhs);
  }

  template <DataUnit U> DataSize<T> operator/(const DataSize<U> &rhs) const {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    return DataSize<T>(m_value / converted_rhs);
  }

  template <DataUnit U> DataSize<T> operator%(const DataSize<U> &rhs) const {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    return DataSize<T>(m_value % converted_rhs);
  }

  template <DataUnit U> DataSize<T> operator+=(const DataSize<U> &rhs) {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    m_value += converted_rhs;
    return *this;
  }

  template <DataUnit U> DataSize<T> operator-=(const DataSize<U> &rhs) {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    m_value -= converted_rhs;
    return *this;
  }

  template <DataUnit U> DataSize<T> operator*=(const DataSize<U> &rhs) {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    m_value *= converted_rhs;
    return *this;
  }

  template <DataUnit U> DataSize<T> operator/=(const DataSize<U> &rhs) {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    m_value /= converted_rhs;
    return *this;
  }

  template <DataUnit U> DataSize<T> operator%=(const DataSize<U> &rhs) {
    uint64_t converted_rhs = rhs.template convert<T>().get_value();
    m_value %= converted_rhs;
    return *this;
  }
  template <DataUnit U> bool operator==(const DataSize<U> &rhs) const {
    return m_value == rhs.template convert<T>().get_value();
  }

  template <DataUnit U> bool operator!=(const DataSize<U> &rhs) const {
    return m_value != rhs.template convert<T>().get_value();
  }

  template <DataUnit U> bool operator<(const DataSize<U> &rhs) const {
    return m_value < rhs.template convert<T>().get_value();
  }

  template <DataUnit U> bool operator>(const DataSize<U> &rhs) const {
    return m_value > rhs.template convert<T>().get_value();
  }

  template <DataUnit U> bool operator<=(const DataSize<U> &rhs) const {
    return m_value <= rhs.template convert<T>().get_value();
  }

  template <DataUnit U> bool operator>=(const DataSize<U> &rhs) const {
    return m_value >= rhs.template convert<T>().get_value();
  }

  template <DataUnit NewDataUnit> DataSize<NewDataUnit> convert() const {
    // std::cout << "m_value: " << m_value << " T: " << static_cast<uint64_t>(T)
    //           << " NewDataUnit: " << static_cast<uint64_t>(NewDataUnit)
    //           << " m_value * static_cast<uint64_t>(T) / static_cast<uint64_t>(NewDataUnit): "
    //           << m_value * static_cast<uint64_t>(T) / static_cast<uint64_t>(NewDataUnit)
    //           << " m_value % static_cast<uint64_t>(NewDataUnit): "
    //           << m_value % static_cast<uint64_t>(NewDataUnit) << "\n";

    if (static_cast<uint64_t>(T) < static_cast<uint64_t>(NewDataUnit)
        && (m_value * static_cast<uint64_t>(T)) % static_cast<uint64_t>(NewDataUnit) != 0
        && !m_allow_rounding) {
      throw std::runtime_error("Cannot convert to a bigger unit with a remainder");
    }

    return DataSize<NewDataUnit>(
        m_value * static_cast<uint64_t>(T) / static_cast<uint64_t>(NewDataUnit), m_allow_rounding);
  }
};