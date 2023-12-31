#pragma once
#include <fmt/core.h>  // or #include <fmt/format.h> based on your fmt version

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

  // Converting constructor
  template <DataUnit U> DataSize(const DataSize<U> &other, bool allow_rounding = false)
      : m_allow_rounding(allow_rounding || other.allow_rounding()) {
    *this = other.template convert<T>();
  }

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
    if (static_cast<uint64_t>(T) < static_cast<uint64_t>(NewDataUnit)
        && (m_value * static_cast<uint64_t>(T)) % static_cast<uint64_t>(NewDataUnit) != 0
        && !m_allow_rounding) {
      throw std::runtime_error("Cannot convert to a bigger unit with a remainder");
    }

    return DataSize<NewDataUnit>(
        m_value * static_cast<uint64_t>(T) / static_cast<uint64_t>(NewDataUnit), m_allow_rounding);
  }

  bool allow_rounding() const { return m_allow_rounding; }
};

template <DataUnit T> struct fmt::formatter<DataSize<T>> {
  // parse is required by the fmt library
  constexpr auto parse(fmt::format_parse_context &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();  // No special parsing needed in this case
  }

  // This is where you define how to format DataSize
  template <typename FormatContext> auto format(const DataSize<T> &size, FormatContext &ctx)
      -> decltype(ctx.out()) {
    std::string unit;
    switch (T) {
      case DataUnit::B:
        unit = "B";
        break;
      case DataUnit::KB:
        unit = "KB";
        break;
      case DataUnit::MB:
        unit = "MB";
        break;
      case DataUnit::GB:
        unit = "GB";
        break;
      case DataUnit::TB:
        unit = "TB";
        break;
      case DataUnit::PB:
        unit = "PB";
        break;
      default:
        throw std::runtime_error("Unknown data unit");
    }
    return fmt::format_to(ctx.out(), "{} {}", size.get_value(), unit);
  }
};
