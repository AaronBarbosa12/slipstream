#include "csv_reader.h"

#include <cmath>
#include <limits>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <charconv>
#include <optional>
#include <stdexcept>

constexpr int NUM_LINES_CSV_HEADER = 21; // Lines to be skipped
constexpr uint64_t PRICE_MULTIPLIER = 10000;
constexpr auto CSV_PATH = SLIPSTREAM_CSV_PATH;
constexpr double INT64_UPPER_BOUND = 0x1p63;  // Exactly 2^63, the exclusive upper bound.

CsvFieldCursor::CsvFieldCursor(std::string_view CSVString): csvString(CSVString) {
    startPosition = 0;
    endPosition = static_cast<int>(csvString.find(','));
}

std::string_view CsvFieldCursor::next() {
    std::string_view next = csvString.substr(startPosition, endPosition - startPosition);
    startPosition = endPosition + 1;
    endPosition = static_cast<int>(csvString.find(',', startPosition + 1));
    return next;
}


void validateParsedField(std::istream& stream, const char* errorMessage) {
    // We fail if the stringstreams fail bit is set OR
    // there are unread characters remaining
    if (stream.fail() || stream.peek() != std::char_traits<char>::eof()) {
        throw std::invalid_argument(errorMessage);
    }
}

uint64_t parseTimeStampString(std::string_view timeStampString) {
    // TODO: Find a way to do this without copying the whole string
    std::istringstream ss{std::string(timeStampString)};
    std::chrono::nanoseconds ts;
    ss >> std::chrono::parse("%H:%M:%S", ts);
    validateParsedField(ss, "Invalid time stamp");
    return static_cast<uint64_t>(ts.count());
}

int64_t parsePriceString(std::string_view priceString) {
    std::istringstream ss{std::string(priceString)};
    double price;
    ss >> price;
    validateParsedField(ss, "Invalid price input");

    // Perform a separate validation to make sure the scaled value will fit in an int64_t
    const double scaledValue = PRICE_MULTIPLIER * price;
    if (!std::isfinite(scaledValue) || scaledValue < -INT64_UPPER_BOUND || scaledValue >= INT64_UPPER_BOUND) {
        throw std::invalid_argument("Price exceeds int64_t range");
    }

    return static_cast<int64_t>(scaledValue);
}

uint32_t parseQuantityString(std::string_view quantityString) {
    std::istringstream ss{std::string(quantityString)};
    uint32_t quantity;
    ss >> quantity;

    validateParsedField(ss, "Invalid bid/trade quantity");

    return quantity;
}

Quote parseQuoteString(std::string_view quoteString) {
    CsvFieldCursor csvStringIterator{quoteString};

    // Extract timestamp
    std::string_view timeStampString = csvStringIterator.next();
    uint64_t ts_ns = parseTimeStampString(timeStampString);

    // Extract symbol
    csvStringIterator.next(); // Skip type
    std::string symbol = static_cast<std::string>(csvStringIterator.next());

    // Extract BidPrice
    std::string_view priceString = csvStringIterator.next();
    int64_t bid_px = parsePriceString(priceString);

    // Extract BidQty
    std::string_view quantityString = csvStringIterator.next();
    uint32_t bid_qty = parseQuantityString(quantityString);

    // Extract AskPrice
    priceString = csvStringIterator.next();
    int64_t ask_px = parsePriceString(priceString);

    // Extract AskQty
    quantityString = csvStringIterator.next();
    uint32_t ask_qty = parseQuantityString(quantityString);

    return Quote{symbol, ts_ns, bid_qty, bid_px, ask_qty, ask_px};
}

Trade parseTradeString(std::string_view tradeString) {
    CsvFieldCursor csvFieldCursor{tradeString};

    // Extract timestamp
    std::string_view timeStampString = csvFieldCursor.next();
    uint64_t ts_ns = parseTimeStampString(timeStampString);

    csvFieldCursor.next(); // Skip type

    // Extract symbol
    std::string symbol = static_cast<std::string>(csvFieldCursor.next());

    csvFieldCursor.next(); // Skip bid price
    csvFieldCursor.next(); // Skip bid qty
    csvFieldCursor.next(); // Skip ask price
    csvFieldCursor.next(); // Skip ask qty

    // Extract price
    std::string_view priceString = csvFieldCursor.next();
    int64_t px = parsePriceString(priceString);

    // Extract qty
    std::string_view quantityString = csvFieldCursor.next();
    uint32_t qty = parseQuantityString(quantityString);

    // Extract aggressor
    std::string_view aggressorStringView = csvFieldCursor.next();

    return Trade{symbol, ts_ns, qty, px, static_cast<std::string>(aggressorStringView)};
}

std::variant<Quote, Trade> parseCSVRow(std::string_view line) {
        // Is the current line a quote or a trade?
        CsvFieldCursor csvFieldCursor{line};
        csvFieldCursor.next(); // skip the timestamp

        std::string_view tradeTypeString = csvFieldCursor.next();
        if (tradeTypeString == "Q") {
            return parseQuoteString(line);
        }
        else if (tradeTypeString == "T") {
            return parseTradeString(line);
        }
        else {
            throw std::runtime_error("Invalid trade type");
        }
}

CsvReader::CsvReader() {
    // Open the quotes and trades .csv
    fileStream_ = std::ifstream(CSV_PATH);
    if (!fileStream_.is_open()) {
        throw std::runtime_error("Cannot open data CSV file");
    }

    // Discard the header
    for (int i = 0; i < NUM_LINES_CSV_HEADER; i++) {
        // Ignore characters on the current line until we reach a \n (which is also discarded)
        fileStream_.ignore(
            std::numeric_limits<std::streamsize>::max(), '\n'
        );
    }
}

std::optional<std::variant<Quote, Trade>> CsvReader::readNextRecord() {
    std::string line;
    if (std::getline(fileStream_, line)) {
        return parseCSVRow(line);
    }
    if (fileStream_.eof() && !fileStream_.bad()) {
        return std::nullopt;
    }
    throw std::runtime_error("Failed to read CSV row");
}
