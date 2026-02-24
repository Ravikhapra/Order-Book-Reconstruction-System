#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

struct Order {
    double price;
    uint64_t size;
    char side; // 'A' for Ask, 'B' for Bid
};

struct PriceLevel {
    uint64_t total_size;
    uint64_t order_count;
};

class OrderBook {
private:
    // Bids: highest price first (descending order)
    std::map<double, PriceLevel, std::greater<double>> bids;
    // Asks: lowest price first (ascending order)  
    std::map<double, PriceLevel> asks;
    // Track orders by ID for cancellations/modifications
    std::unordered_map<uint64_t, Order> orders;
    
    // Helper methods
    void add_order(uint64_t order_id, double price, uint64_t size, char side);
    void cancel_order(uint64_t order_id, uint64_t size);
    
public:
    OrderBook();
    ~OrderBook();
    
    // Main processing method
    void process_mbo_action(const std::string& line, std::string& output_line);
    
    // Process T->F->C sequence as single action
    void process_tfc_sequence(const std::string& t_line, const std::string& f_line, const std::string& c_line, std::string& output_line);
    
    // Check if action affects top 10 levels
    bool affects_top10_levels(char action, char side, double price) const;
    
    // Generate MBP-10 snapshot
    std::string get_mbp_10_snapshot(const std::vector<std::string>& mbo_fields, uint64_t row_index, int depth = 0) const;
    
    // Clear the orderbook
    void clear();
};

#endif // ORDERBOOK_H
