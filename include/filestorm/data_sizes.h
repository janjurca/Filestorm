#pragma once
#include <filestorm/utils.h>
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
  DataSize() : m_value(0), m_allow_rounding(false) {}
  DataSize(uint64_t value, bool allow_rounding = false) : m_value(value), m_allow_rounding(allow_rounding) {}

  // Converting constructor
  template <DataUnit U> DataSize(const DataSize<U> &other, bool allow_rounding = false) : m_allow_rounding(allow_rounding || other.allow_rounding()) { *this = other.template convert<T>(); }

  uint64_t get_value() const { return m_value; }

// Macro to generate arithmetic operators handling different units
#define DEFINE_ARITHMETIC_OPERATOR(op)                                          \
  template <DataUnit U> DataSize<T> operator op(const DataSize<U> &rhs) const { \
    uint64_t converted_rhs = rhs.template convert<T>().get_value();             \
    return DataSize<T>(m_value op converted_rhs);                               \
  }                                                                             \
  template <DataUnit U> DataSize<T> &operator op##=(const DataSize<U> &rhs) {   \
    uint64_t converted_rhs = rhs.template convert<T>().get_value();             \
    m_value = m_value op converted_rhs;                                         \
    return *this;                                                               \
  }                                                                             \
  template <typename NumericType> DataSize<T> operator op(const NumericType & rhs) const { return DataSize<T>(m_value op rhs); }

  DEFINE_ARITHMETIC_OPERATOR(+)
  DEFINE_ARITHMETIC_OPERATOR(-)
  DEFINE_ARITHMETIC_OPERATOR(*)
  DEFINE_ARITHMETIC_OPERATOR(/)
  DEFINE_ARITHMETIC_OPERATOR(%)

#undef DEFINE_ARITHMETIC_OPERATOR

#define DEFINE_COMPARISON_OPERATOR(op) \
  template <DataUnit U> bool operator op(const DataSize<U> &rhs) const { return m_value op rhs.template convert<T>().get_value(); }

  DEFINE_COMPARISON_OPERATOR(==)
  DEFINE_COMPARISON_OPERATOR(!=)
  DEFINE_COMPARISON_OPERATOR(<)
  DEFINE_COMPARISON_OPERATOR(>)
  DEFINE_COMPARISON_OPERATOR(<=)
  DEFINE_COMPARISON_OPERATOR(>=)

#undef DEFINE_COMPARISON_OPERATOR

  template <DataUnit U> DataSize<U> zero() { return DataSize<U>(0); }

  template <DataUnit NewDataUnit> DataSize<NewDataUnit> convert() const {
    if (static_cast<uint64_t>(T) < static_cast<uint64_t>(NewDataUnit) && (m_value * static_cast<uint64_t>(T)) % static_cast<uint64_t>(NewDataUnit) != 0 && !m_allow_rounding) {
      throw std::runtime_error("Cannot convert to a bigger unit with a remainder");
    }

    return DataSize<NewDataUnit>(m_value * static_cast<uint64_t>(T) / static_cast<uint64_t>(NewDataUnit), m_allow_rounding);
  }

  bool allow_rounding() const { return m_allow_rounding; }

  static DataSize<T> fromString(const std::string &str) {
    std::string value_str;
    std::string unit_str;

    bool reading_number = true;
    for (auto &c : str) {
      if ((std::isdigit(c) || c == '.')) {
        if (reading_number == false) {
          throw std::runtime_error(fmt::format("Invalid data size string: {}", str));
        }
        value_str += c;
      } else {
        unit_str += c;
        reading_number = false;
      }
    }

    if (value_str.empty() || unit_str.empty()) {
      throw std::runtime_error(fmt::format("Invalid data size string: {}", str));
    }

    unit_str = toUpper(unit_str);
    double value = std::stod(value_str);
    bool hasDecimalPlaces = value != std::floor(value);
    if (hasDecimalPlaces) {
      throw std::runtime_error(fmt::format("Invalid data size string (decimal places are not supported): {}", str));
    }

    uint64_t bytes;

    if (unit_str == "B") {
      bytes = static_cast<uint64_t>(value);
    } else if (unit_str == "KB" || unit_str == "K") {
      bytes = static_cast<uint64_t>(value) * static_cast<uint64_t>(DataUnit::KB);
    } else if (unit_str == "MB" || unit_str == "M") {
      bytes = static_cast<uint64_t>(value) * static_cast<uint64_t>(DataUnit::MB);
    } else if (unit_str == "GB" || unit_str == "G") {
      bytes = static_cast<uint64_t>(value) * static_cast<uint64_t>(DataUnit::GB);
    } else if (unit_str == "TB" || unit_str == "T") {
      bytes = static_cast<uint64_t>(value) * static_cast<uint64_t>(DataUnit::TB);
    } else if (unit_str == "PB" || unit_str == "P") {
      bytes = static_cast<uint64_t>(value) * static_cast<uint64_t>(DataUnit::PB);
    } else {
      throw std::runtime_error(fmt::format("Invalid data size unit: {}", unit_str));
    }
    uint64_t unit_size = static_cast<uint64_t>(T);
    if (bytes % unit_size != 0) {
      throw std::runtime_error(fmt::format("Invalid data size string (cannot convert to the specified unit without a remainder): {}", str));
    }
    return DataSize<T>(bytes / unit_size, false);
  }
};

template <DataUnit T> struct fmt::formatter<DataSize<T>> {
  // parse is required by the fmt library
  constexpr auto parse(fmt::format_parse_context &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();  // No special parsing needed in this case
  }

  // This is where you define how to format DataSize
  template <typename FormatContext> auto format(const DataSize<T> &size, FormatContext &ctx) const -> decltype(ctx.out()) {
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
