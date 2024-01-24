#include <doctest/doctest.h>
#include <filestorm/data_sizes.h>

#include <iostream>
#include <string>

TEST_CASE("Construction and get_value") {
  DataSize<DataUnit::KB> kbSize(1024);
  CHECK(kbSize.get_value() == 1024);
}

TEST_CASE("Arithmetic operations") {
  DataSize<DataUnit::KB> kbSize1(1024);
  DataSize<DataUnit::KB> kbSize2(2048);

  SUBCASE("Addition") {
    auto result = kbSize1 + kbSize2;
    CHECK(result.get_value() == 3072);
  }

  SUBCASE("Subtraction") {
    auto result = kbSize2 - kbSize1;
    CHECK(result.get_value() == 1024);
  }

  // Add more cases for multiplication, division, and modulo
}

TEST_CASE("Arithmetic operations with different units") {
  DataSize<DataUnit::KB> kbSize(1024);
  DataSize<DataUnit::MB> mbSize(1);

  SUBCASE("Addition") {
    auto result = kbSize + mbSize;  // Should convert MB to KB
    CHECK(result.get_value() == 2048);
  }

  // Add more cases for subtraction, multiplication, division with different units
}

TEST_CASE("Comparison operations") {
  DataSize<DataUnit::KB> kbSize1(1024);
  DataSize<DataUnit::KB> kbSize2(2048);

  SUBCASE("Equality") {
    CHECK(kbSize1 == kbSize1);
    CHECK_FALSE(kbSize1 == kbSize2);
  }

  SUBCASE("Less than") { CHECK(kbSize1 < kbSize2); }

  // Add more cases for !=, >, <=, >=
}

TEST_CASE("Conversion") {
  DataSize<DataUnit::MB> mbSize(1);  // 1 MB

  SUBCASE("Convert to KB") {
    auto kbSize = mbSize.convert<DataUnit::KB>();
    CHECK(kbSize.get_value() == 1024);
  }

  // Add more cases for conversion to other units
}
TEST_CASE("DataSize Constructor and get_value()") {
  DataSize<DataUnit::KB> kilobytes(1024);
  CHECK(kilobytes.get_value() == 1024);
}

TEST_CASE("DataSize Arithmetic Operations") {
  SUBCASE("Addition with different units") {
    DataSize<DataUnit::MB> megabytes(1);
    DataSize<DataUnit::KB> kilobytes(1024);
    auto result = megabytes + kilobytes;
    CHECK(result.get_value() == 2);
  }

  SUBCASE("Subtraction with different units") {
    DataSize<DataUnit::MB> megabytes(2);
    DataSize<DataUnit::KB> kilobytes(1024);
    auto result = megabytes - kilobytes;
    CHECK(result.get_value() == 1);
  }

  // Add more cases for multiplication, division, and modulus
}
TEST_CASE("DataSize Assignment Operations") {
  DataSize<DataUnit::MB> megabytes(2);
  DataSize<DataUnit::KB> kilobytes(1024);

  megabytes += kilobytes;
  CHECK(megabytes.get_value() == 3);

  megabytes -= kilobytes;
  CHECK(megabytes.get_value() == 2);

  // Add more cases for *=, /=, and %=
}

TEST_CASE("DataSize Comparison Operations") {
  DataSize<DataUnit::GB> gigabytes(1);
  DataSize<DataUnit::MB> megabytes(1024);

  CHECK(gigabytes == megabytes);
  CHECK(gigabytes >= megabytes);
  CHECK(megabytes <= gigabytes);

  // Add more cases for !=, >, and <
}

TEST_CASE("DataSize Unit Conversion") {
  DataSize<DataUnit::GB> gigabytes(1);
  auto converted = gigabytes.convert<DataUnit::MB>();
  CHECK(converted.get_value() == 1024);

  // Add more cases for other unit conversions
}

TEST_CASE("DataSize Unit Conversion Tests") {
  SUBCASE("Conversion from smaller to larger units") {
    DataSize<DataUnit::B> bytes(1024);
    auto kilobytes = bytes.convert<DataUnit::KB>();
    CHECK(kilobytes.get_value() == 1);

    CHECK_THROWS(bytes.convert<DataUnit::MB>());
  }
  SUBCASE("Conversion from smaller to larger units with allow rounding") {
    DataSize<DataUnit::B> bytes(1024, true);
    auto kilobytes = bytes.convert<DataUnit::KB>();
    CHECK(kilobytes.get_value() == 1);

    auto megabytes = bytes.convert<DataUnit::MB>();
    CHECK(megabytes.get_value() == 0);
  }

  SUBCASE("Conversion from larger to smaller units") {
    DataSize<DataUnit::TB> terabytes(1);
    auto gigabytes = terabytes.convert<DataUnit::GB>();
    CHECK(gigabytes.get_value() == 1024);

    auto megabytes = terabytes.convert<DataUnit::MB>();
    CHECK(megabytes.get_value() == 1048576);
  }

  SUBCASE("Conversion with the same unit") {
    DataSize<DataUnit::GB> gigabytes(5);
    auto converted = gigabytes.convert<DataUnit::GB>();
    CHECK(converted.get_value() == 5);
  }

  SUBCASE("Mixed unit arithmetic followed by conversion") {
    DataSize<DataUnit::GB> gigabytes(2);
    DataSize<DataUnit::MB> megabytes(1024);  // 1 GB in MB

    auto result = gigabytes + megabytes;  // Result is in GB
    auto convertedResult = result.convert<DataUnit::MB>();
    CHECK(convertedResult.get_value() == 3072);  // 3 GB in MB
  }

  SUBCASE("Arithmetic operations on converted units") {
    DataSize<DataUnit::GB> gigabytes(1);
    auto megabytes = gigabytes.convert<DataUnit::MB>();

    DataSize<DataUnit::MB> additionalMegabytes(512);
    auto result = megabytes + additionalMegabytes;
    CHECK(result.get_value() == 1536);  // 1.5 GB in MB
  }

  SUBCASE("Complex operations involving multiple conversions") {
    DataSize<DataUnit::GB> gigabytes(2);
    DataSize<DataUnit::MB> megabytes(500);
    DataSize<DataUnit::KB> kilobytes(1024);

    // Convert all to bytes and perform operations
    auto gb_in_bytes = gigabytes.convert<DataUnit::B>();
    auto mb_in_bytes = megabytes.convert<DataUnit::B>();
    auto kb_in_bytes = kilobytes.convert<DataUnit::B>();
    auto totalBytes = mb_in_bytes + kb_in_bytes + gb_in_bytes;
    auto totalGB = totalBytes.convert<DataUnit::KB>();

    CHECK(totalGB.get_value() > 2);  // Result should be more than 2 GB
  }

  // Additional cases can be added for testing edge cases and error scenarios
}

TEST_CASE("DataSize Unit Conversion fractional") {
  SUBCASE("convert to a bigger unit with fractional value") {
    DataSize<DataUnit::GB> gigabytes(2);
    DataSize<DataUnit::MB> megabytes(500);  // 1 GB in MB

    CHECK_THROWS(gigabytes + megabytes);
  }

  // Add more cases for other unit conversions
}

TEST_CASE("DataSize fromString") {
  // Test valid inputs
  CHECK(DataSize<DataUnit::B>::fromString("123B") == DataSize<DataUnit::B>(123));
  CHECK(DataSize<DataUnit::KB>::fromString("456KB") == DataSize<DataUnit::KB>(456));
  CHECK(DataSize<DataUnit::MB>::fromString("789M") == DataSize<DataUnit::MB>(789));
  CHECK(DataSize<DataUnit::GB>::fromString("10G") == DataSize<DataUnit::GB>(10));
  CHECK(DataSize<DataUnit::TB>::fromString("11TB") == DataSize<DataUnit::TB>(11));
  CHECK(DataSize<DataUnit::PB>::fromString("12PB") == DataSize<DataUnit::PB>(12));

  // Test valid inputs with lowercase units
  CHECK(DataSize<DataUnit::KB>::fromString("13kb") == DataSize<DataUnit::KB>(13));
  CHECK(DataSize<DataUnit::MB>::fromString("14mb") == DataSize<DataUnit::MB>(14));
  CHECK(DataSize<DataUnit::GB>::fromString("15gb") == DataSize<DataUnit::GB>(15));
  CHECK(DataSize<DataUnit::TB>::fromString("16tb") == DataSize<DataUnit::TB>(16));
  CHECK(DataSize<DataUnit::PB>::fromString("17pb") == DataSize<DataUnit::PB>(17));

  // Test invalid inputs
  CHECK_THROWS_AS(DataSize<DataUnit::B>::fromString("invalid"), std::runtime_error);
  CHECK_THROWS_AS(DataSize<DataUnit::B>::fromString("123"), std::runtime_error);
  CHECK_THROWS_AS(DataSize<DataUnit::B>::fromString("B123"), std::runtime_error);
  CHECK_THROWS_AS(DataSize<DataUnit::B>::fromString("1.5B"), std::runtime_error);
  CHECK_THROWS_AS(DataSize<DataUnit::B>::fromString("123 MB"), std::runtime_error);
  CHECK_THROWS_AS(DataSize<DataUnit::B>::fromString("invalid unit"), std::runtime_error);
}