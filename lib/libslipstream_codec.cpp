#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#include <chrono>
#include <cstring>

constexpr int NUM_LINES_CSV_HEADER = 21; // Lines to be skipped
//constexpr int NUM_VALUES_PER_LINE = 10; //Timestamp,Type,Symbol,BidPrice,BidQty,AskPrice,AskQty,Price,Qty,Aggressor
constexpr uint64_t PRICE_MULTIPLIER = 10000;
const std::string CSV_PATH = "C:/Users/Aaron B/CLionProjects/slipstream/data/QandT.csv";

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

class CSVStringIterator {
    public:
        CSVStringIterator(std::string_view CSVString): csvString(CSVString) {
            startPosition = 0;
            endPosition = static_cast<int>(csvString.find(','));
        }

        std::string_view next() {
            std::string_view next = csvString.substr(startPosition, endPosition - startPosition);

            // Update start and end positions
            startPosition = endPosition + 1;
            endPosition = static_cast<int>(csvString.find(',', startPosition + 1));
            return next;
        }

    private:
        std::string_view csvString;
        int startPosition;
        int endPosition;
};

uint64_t parseTimeStampString(std::string_view timeStampString) {
    // TODO: Find a way to do this without copying the whole string
    std::istringstream ss{std::string(timeStampString)};
    std::chrono::nanoseconds ts;
    ss >> std::chrono::parse("%H:%M:%S", ts); // supports fractional seconds
    return static_cast<uint64_t>(ts.count());
}

int64_t parsePriceString(std::string_view priceString) {
    std::istringstream ss{std::string(priceString)};
    double price;
    ss >> price;
    return static_cast<int64_t>(PRICE_MULTIPLIER * price);
}

uint64_t parseQuantityString(std::string_view quantityString) {
    std::istringstream ss{std::string(quantityString)};
    uint64_t quantity;
    ss >> quantity;
    return quantity;
}

Quote parseQuoteString(std::string& quoteString) {
    CSVStringIterator csvStringIterator{quoteString};

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
    uint64_t bid_qty = parseQuantityString(quantityString);

    // Extract AskPrice
    priceString = csvStringIterator.next();
    int64_t ask_px = parsePriceString(priceString);

    // Extract AskQty
    quantityString = csvStringIterator.next();
    uint64_t ask_qty = parseQuantityString(quantityString);

    return Quote{symbol, ts_ns, bid_qty, bid_px, ask_qty, ask_px};
}

Trade parseTradeString(std::string& tradeString) {
    CSVStringIterator csvStringIterator{tradeString};

    // Extract timestamp
    std::string_view timeStampString = csvStringIterator.next();
    uint64_t ts_ns = parseTimeStampString(timeStampString);

    csvStringIterator.next(); // Skip type

    // Extract symbol
    std::string symbol = static_cast<std::string>(csvStringIterator.next());

    csvStringIterator.next(); // Skip bid price
    csvStringIterator.next(); // Skip bid qty
    csvStringIterator.next(); // Skip ask price
    csvStringIterator.next(); // Skip ask qty

    // Extract price
    std::string_view priceString = csvStringIterator.next();
    int64_t px = parsePriceString(priceString);

    // Extract qty
    std::string_view quantityString = csvStringIterator.next();
    uint64_t qty = parseQuantityString(quantityString);

    // Extract aggressor
    std::string_view aggressorStringView = csvStringIterator.next();

    return Trade{symbol, ts_ns, qty, px, static_cast<std::string>(aggressorStringView)};
}

int main() {
    std::ifstream fileStream(CSV_PATH);
    if (!fileStream.is_open()) {
        throw std::runtime_error("Cannot open data CSV file");
    }

    // Discard the header
    for (int i = 0; i < NUM_LINES_CSV_HEADER; i++) {
        // Ignore characters on the current line until we reach a \n (which is also discarded)
        fileStream.ignore(
            std::numeric_limits<std::streamsize>::max(), '\n'
        );
    }

    std::string currentLine;
    while (std::getline(fileStream, currentLine)) {
        /*
         * Read characters from fileStream until we encounter \n (newline) or EOF.
         * Advance the stream's current position to the beginning of the next line.
         */
        // Is the current line a quote or a trade?
        CSVStringIterator csvStringIterator{currentLine};
        csvStringIterator.next(); // skip the timestamp
        std::string_view tradeTypeString = csvStringIterator.next();

        if (tradeTypeString == "Q") {
            Quote quote = parseQuoteString(currentLine);
        }
        else if (tradeTypeString == "T") {
            Trade trade = parseTradeString(currentLine);
        }
        else {
            throw std::runtime_error("Invalid trade type");
        }
    }
}
