#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "../lib/csv_reader.h"

TEST(CsvReaderTest, TestparseTimeStampString) {
    EXPECT_EQ(parseTimeStampString("09:30:00.003"), 34'200'003'000'000ULL);

    // Corrupted timestamp strings
    EXPECT_THROW(parseTimeStampString("09:apple30:00.003"), std::invalid_argument);
    EXPECT_THROW(parseTimeStampString("09:30:00.003apple"), std::invalid_argument);
    EXPECT_THROW(parseTimeStampString("09:3sadasd0:00.00dsadsa3apple"), std::invalid_argument);
    EXPECT_THROW(parseTimeStampString("asdasdasa"), std::invalid_argument);
}

TEST(CsvReaderTest, TestparsePriceString) {
    EXPECT_EQ(parsePriceString("87.23"), 87.23*10'000);

    // Corrupted Price strings
    EXPECT_THROW(parsePriceString("87.23sadasdas"), std::invalid_argument);
    EXPECT_THROW(parsePriceString("crab  "), std::invalid_argument);
    EXPECT_THROW(parsePriceString("3_"), std::invalid_argument);


    // Check for overflow
    // Previous approach - WRONG
    // constexpr long double overflowingPrice = std::numeric_limits<double>::max() + 1;
    // EXPECT_THROW(parsePriceString(std::to_string(overflowingPrice)), std::invalid_argument);
    // Adding epsilon() to the largest double rounded back to that same value:
    // epsilon() is the spacing near 1.0, and the spacing near max() is much larger.

    // Check if the input string is too large of a floating point value
    const auto overflowingExponent = std::numeric_limits<double>::max_exponent10 + 1;
    EXPECT_THROW(parsePriceString("1e" + std::to_string(overflowingExponent)), std::invalid_argument);

    // What if the input string fits into double, but price x 10'000 doesn't fit into int64_t?
    constexpr double maxPrice = std::numeric_limits<std::int64_t>::max();
    constexpr double firstOverflowingWholePrice = maxPrice / 10'000 + 1;

    EXPECT_THROW(parsePriceString(std::to_string(firstOverflowingWholePrice)), std::invalid_argument);
}

TEST(CsvReaderTest, TestQuantityString) {
    EXPECT_EQ(parseQuantityString("55"), 55u);

    // Corrupted Price strings
    EXPECT_THROW(parseQuantityString("55sadasdas"), std::invalid_argument);
    EXPECT_THROW(parseQuantityString("432432.12"), std::invalid_argument);
    EXPECT_THROW(parseQuantityString("apple"), std::invalid_argument);

    constexpr std::uint64_t maxQuantity = std::numeric_limits<std::uint32_t>::max();
    EXPECT_THROW(parseQuantityString(std::to_string(maxQuantity + 1)), std::invalid_argument);
}
