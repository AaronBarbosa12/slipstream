#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

struct Quote {
    std::string symbol; // TODO: DO We need to make this into a char?
    uint64_t ts_ns; // Nanoseconds since Unix epoch
    uint32_t bid_qty;
    int64_t bid_px; // Fixed point X 10,000
    uint32_t ask_qty;
    int64_t ask_px; // Fixed point X 10,000
};

struct Trade {
    std::string symbol;  // TODO: DO We need to make this into a char?
    uint64_t ts_ns; // Nanoseconds since Unix epoch
    uint32_t qty;
    int64_t px; // Fixed point X 10,000
    std::string aggressor; // 'B', 'S', or '?' if unknown, TODO: DO We need to make this into a char? TODO: make into enum?
    //int64_t id; TODO: ImplementTrade identifier
};

class CsvReader {
public:
    CsvReader();
    std::optional<std::variant<Quote, Trade>> readNextRecord();

private:
    std::ifstream fileStream_;
};

class CsvFieldCursor {
public:
    CsvFieldCursor(std::string_view CSVString);
    std::string_view next();

private:
    std::string_view csvString;
    int startPosition;
    int endPosition;
};

uint64_t parseTimeStampString(std::string_view timeStampString);
int64_t parsePriceString(std::string_view priceString);
uint32_t parseQuantityString(std::string_view quantityString);
Quote parseQuoteString(std::string_view quoteString);
Trade parseTradeString(std::string_view tradeString);
std::variant<Quote, Trade> parseCSVRow(std::string_view line);
