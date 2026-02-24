#include "../include/orderbook.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cmath>

OrderBook::OrderBook() {
    // Constructor - maps are automatically initialized
}

OrderBook::~OrderBook() {
    // Destructor - maps will be automatically cleaned up
}

void OrderBook::clear() {
    bids.clear();
    asks.clear();
    orders.clear();
}

void OrderBook::add_order(uint64_t order_id, double price, uint64_t size, char side) {
    // Store the order
    orders[order_id] = {price, size, side};
    
    // Update the appropriate price level
    if (side == 'B') {
        if (bids.find(price) == bids.end()) {
            // New price level
            bids[price] = {size, 1};
        } else {
            // Existing price level - increment both size and count
            bids[price].total_size += size;
            bids[price].order_count += 1;
        }
    } else if (side == 'A') {
        if (asks.find(price) == asks.end()) {
            // New price level
            asks[price] = {size, 1};
        } else {
            // Existing price level - increment both size and count
            asks[price].total_size += size;
            asks[price].order_count += 1;
        }
    }
}

void OrderBook::cancel_order(uint64_t order_id, uint64_t size) {
    auto order_it = orders.find(order_id);
    if (order_it == orders.end()) {
        return; // Order not found
    }
    
    Order& order = order_it->second;
    double price = order.price;
    char side = order.side;
    uint64_t original_order_size = order.size;
    
    // Determine if this is a complete or partial cancellation
    bool is_complete_cancellation = (size >= original_order_size);
    uint64_t actual_canceled_size = std::min(size, original_order_size);
    
    // Update the price level
    if (side == 'B') {
        auto bid_it = bids.find(price);
        if (bid_it != bids.end()) {
            bid_it->second.total_size -= actual_canceled_size;
            if (is_complete_cancellation) {
                // Complete order cancellation - decrement count
                bid_it->second.order_count -= 1;
            }
            if (bid_it->second.total_size == 0) {
                bids.erase(bid_it);
            }
        }
    } else if (side == 'A') {
        auto ask_it = asks.find(price);
        if (ask_it != asks.end()) {
            ask_it->second.total_size -= actual_canceled_size;
            if (is_complete_cancellation) {
                // Complete order cancellation - decrement count
                ask_it->second.order_count -= 1;
            }
            if (ask_it->second.total_size == 0) {
                asks.erase(ask_it);
            }
        }
    }
    
    // Update or remove the order
    if (is_complete_cancellation) {
        orders.erase(order_it);
    } else {
        order.size -= actual_canceled_size;
    }
}

std::string OrderBook::get_mbp_10_snapshot(const std::vector<std::string>& mbo_fields, uint64_t row_index, int depth) const {
    // Step 1: Declare fixed-size vector for exactly 76 columns
    std::vector<std::string> output_row(76, "");
    
    // Initialize price fields as empty, size/count fields as "0"
    for (int i = 14; i < 74; i++) {
        if ((i - 14) % 6 == 1 || (i - 14) % 6 == 2 || (i - 14) % 6 == 4 || (i - 14) % 6 == 5) {
            output_row[i] = "0";  // size and count fields
        }
        // price fields remain empty string by default
    }
    
    // Step 2: Populate initial MBO data fields (columns 0-13)
    std::string ts_recv = mbo_fields[0];
    std::string ts_event = mbo_fields[1];
    char action = mbo_fields[5][0];
    char side = mbo_fields[6][0];
    std::string price_str = mbo_fields[7];
    std::string size_str = mbo_fields[8];
    std::string flags = mbo_fields[11];
    std::string ts_in_delta = mbo_fields[12];
    std::string sequence = mbo_fields[13];
    std::string symbol = mbo_fields[14];
    std::string order_id = mbo_fields[10];
    
    // Clean and trim all string fields
    auto clean_field = [](std::string& field) {
        size_t start = field.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            field = "";
            return;
        }
        size_t end = field.find_last_not_of(" \t\r\n");
        field = field.substr(start, end - start + 1);
    };
    
    clean_field(ts_event);
    clean_field(price_str);
    clean_field(size_str);
    clean_field(flags);
    clean_field(ts_in_delta);
    clean_field(sequence);
    clean_field(symbol);
    clean_field(order_id);
    
    // Format price to remove trailing zeros
    if (!price_str.empty() && price_str.find('.') != std::string::npos) {
        try {
            double price_val = std::stod(price_str);
            std::ostringstream price_oss;
            price_oss << price_val;
            price_str = price_oss.str();
            
            if (price_str.find('.') != std::string::npos) {
                price_str = price_str.substr(0, price_str.find_last_not_of('0') + 1);
                if (price_str.back() == '.') {
                    price_str.pop_back();
                }
            }
        } catch (...) {
            // Keep original string if parsing fails
        }
    }
    
    // Populate columns 0-13 with MBO metadata
    output_row[0] = std::to_string(row_index);
    output_row[1] = ts_event;
    output_row[2] = ts_event;
    output_row[3] = "10";
    output_row[4] = "2";
    output_row[5] = "1108";
    output_row[6] = std::string(1, action);
    output_row[7] = std::string(1, side);
    output_row[8] = std::to_string(depth);
    output_row[9] = price_str;
    output_row[10] = size_str;
    output_row[11] = flags;
    output_row[12] = ts_in_delta;
    output_row[13] = sequence;
    
    // Step 3: Populate the 60 MBP-10 fields (columns 14-73)
    // Format: bid_px_00,bid_sz_00,bid_ct_00,ask_px_00,ask_sz_00,ask_ct_00 (repeated 10 times)
    
    // Step 3a: Iterate through bid levels (up to 10)
    int bid_level = 0;
    for (auto it = bids.begin(); it != bids.end() && bid_level < 10; ++it, ++bid_level) {
        double price = it->first;
        std::ostringstream price_oss;
        price_oss << price;
        std::string formatted_price = price_oss.str();
        
        // Remove trailing zeros from price
        if (formatted_price.find('.') != std::string::npos) {
            formatted_price = formatted_price.substr(0, formatted_price.find_last_not_of('0') + 1);
            if (formatted_price.back() == '.') {
                formatted_price.pop_back();
            }
        }
        
        // Calculate correct indices: each level has 6 fields (bid_px, bid_sz, bid_ct, ask_px, ask_sz, ask_ct)
        int base_index = 14 + (bid_level * 6);
        output_row[base_index + 0] = formatted_price;                           // bid_px_XX
        output_row[base_index + 1] = std::to_string(it->second.total_size);     // bid_sz_XX
        output_row[base_index + 2] = std::to_string(it->second.order_count);    // bid_ct_XX
    }
    
    // Step 3b: Iterate through ask levels (up to 10)
    int ask_level = 0;
    for (auto it = asks.begin(); it != asks.end() && ask_level < 10; ++it, ++ask_level) {
        double price = it->first;
        std::ostringstream price_oss;
        price_oss << price;
        std::string formatted_price = price_oss.str();
        
        // Remove trailing zeros from price
        if (formatted_price.find('.') != std::string::npos) {
            formatted_price = formatted_price.substr(0, formatted_price.find_last_not_of('0') + 1);
            if (formatted_price.back() == '.') {
                formatted_price.pop_back();
            }
        }
        
        // Calculate correct indices: ask fields are offset by 3 from bid fields  
        int base_index = 14 + (ask_level * 6);
        output_row[base_index + 3] = formatted_price;                           // ask_px_XX
        output_row[base_index + 4] = std::to_string(it->second.total_size);     // ask_sz_XX
        output_row[base_index + 5] = std::to_string(it->second.order_count);    // ask_ct_XX
    }
    
    // Step 4: Populate final data fields (columns 74-75)
    output_row[74] = symbol;     // symbol
    output_row[75] = order_id;   // order_id
    
    // Step 5: Join all 76 elements with commas to create output string
    std::ostringstream result;
    for (int i = 0; i < 76; i++) {
        result << output_row[i];
        if (i < 75) result << ",";  // Add comma except after the last field
    }
    
    return result.str();
}

void OrderBook::process_tfc_sequence(const std::string& t_line, const std::string& f_line, const std::string& c_line, std::string& output_line) {
    // Parse the C action to get the actual order book impact
    std::vector<std::string> c_fields;
    std::stringstream c_ss(c_line);
    std::string field;
    
    while (std::getline(c_ss, field, ',')) {
        c_fields.push_back(field);
    }
    
    if (c_fields.size() < 15) {
        output_line = "";
        return;
    }
    
    // Parse T action for output formatting
    std::vector<std::string> t_fields;
    std::stringstream t_ss(t_line);
    
    while (std::getline(t_ss, field, ',')) {
        t_fields.push_back(field);
    }
    
    if (t_fields.size() < 15) {
        output_line = "";
        return;
    }
    
    // Use C action details for order book modification (this affects the book)
    char c_side = c_fields[6][0];
    uint64_t order_id = 0;
    uint64_t size = 0;
    
    try {
        if (!c_fields[8].empty()) size = std::stoull(c_fields[8]);
        if (!c_fields[10].empty()) order_id = std::stoull(c_fields[10]);
    } catch (const std::exception& e) {
        output_line = "";
        return;
    }
    
    // Skip if side is 'N' (except for clear action and trade actions)
    if (c_side == 'N') {
        output_line = "";
        return;
    }
    
    // Calculate depth BEFORE applying the cancellation
    int depth = 0;
    double c_price = 0.0;
    try {
        if (!c_fields[7].empty()) c_price = std::stod(c_fields[7]);
    } catch (const std::exception& e) {
        output_line = "";
        return;
    }
    
    // Find current position of the price level for depth calculation
    if (c_side == 'B') {
        int level = 0;
        for (auto it = bids.begin(); it != bids.end(); ++it, ++level) {
            if (std::fabs(it->first - c_price) < 1e-9) {
                depth = level;
                break;
            }
        }
    } else if (c_side == 'A') {
        int level = 0;
        for (auto it = asks.begin(); it != asks.end(); ++it, ++level) {
            if (std::fabs(it->first - c_price) < 1e-9) {
                depth = level;
                break;
            }
        }
    }
    
    // Apply the cancellation to the order book
    cancel_order(order_id, size);
    
    // Generate output using T action fields but with corrected side and depth
    // According to requirement: "we store the T action on the BID side as that is the side whose change is actually reflected in the book"
    // So we use the C action's side (the side that actually changes) for the output
    std::vector<std::string> output_fields = t_fields;
    output_fields[6] = std::string(1, c_side);  // Use the side that actually changed
    
    // Generate MBP-10 snapshot with the T action metadata but correct side
    output_line = get_mbp_10_snapshot(output_fields, 0, depth);
}

bool OrderBook::affects_top10_levels(char action, char side, double price) const {
    if (action == 'R' || action == 'T') {
        return true; // Reset and Trade actions always generate output
    }
    
    if (side == 'B') {
        // For bids, check if price is in top 10 levels
        int level = 0;
        for (auto it = bids.begin(); it != bids.end() && level < 10; ++it, ++level) {
            if (std::fabs(it->first - price) < 1e-9) {
                return true; // Price found in top 10
            }
        }
        
        // For Add actions, check if it would be inserted in top 10
        if (action == 'A') {
            level = 0;
            for (auto it = bids.begin(); it != bids.end() && level < 10; ++it, ++level) {
                if (price > it->first) {
                    return true; // Would be inserted in top 10
                }
            }
            return level < 10; // Would be added at the end if less than 10 levels
        }
    } else if (side == 'A') {
        // For asks, check if price is in top 10 levels
        int level = 0;
        for (auto it = asks.begin(); it != asks.end() && level < 10; ++it, ++level) {
            if (std::fabs(it->first - price) < 1e-9) {
                return true; // Price found in top 10
            }
        }
        
        // For Add actions, check if it would be inserted in top 10
        if (action == 'A') {
            level = 0;
            for (auto it = asks.begin(); it != asks.end() && level < 10; ++it, ++level) {
                if (price < it->first) {
                    return true; // Would be inserted in top 10
                }
            }
            return level < 10; // Would be added at the end if less than 10 levels
        }
    }
    
    return false; // Doesn't affect top 10 levels
}

void OrderBook::process_mbo_action(const std::string& line, std::string& output_line) {
    // Parse CSV line - using simple string parsing for performance
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    
    if (fields.size() < 15) {
        output_line = "";
        return;
    }
    
    char action = fields[5][0];
    char side = fields[6][0];
    uint64_t order_id = 0;
    double price = 0.0;
    uint64_t size = 0;
    
    // Parse numeric fields
    try {
        if (!fields[7].empty()) price = std::stod(fields[7]);
        if (!fields[8].empty()) size = std::stoull(fields[8]);
        if (!fields[10].empty()) order_id = std::stoull(fields[10]);
    } catch (const std::exception& e) {
        output_line = "";
        return;
    }
    
    // Skip if side is 'N' (except for clear action and trade actions)
    // COMPANY REQUIREMENT: Actually process all actions including side='N'
    // if (side == 'N' && action != 'R' && action != 'T') {
    //     output_line = "";
    //     return;
    // }
    
    // COMPANY REQUIREMENT: Store top-of-book before action
    double prev_best_bid = (bids.empty()) ? 0.0 : bids.rbegin()->first;
    double prev_best_ask = (asks.empty()) ? 0.0 : asks.begin()->first;
    size_t prev_bid_count = bids.size();
    size_t prev_ask_count = asks.size();
    
    // Calculate depth BEFORE applying the action
    int depth = 0;
    if (action == 'C' && side != 'N') {
        // For cancel, find current position of the price level
        if (side == 'B') {
            int level = 0;
            for (auto it = bids.begin(); it != bids.end(); ++it, ++level) {
                if (std::fabs(it->first - price) < 1e-9) {
                    depth = level;
                    break;
                }
            }
        } else if (side == 'A') {
            int level = 0;
            for (auto it = asks.begin(); it != asks.end(); ++it, ++level) {
                if (std::fabs(it->first - price) < 1e-9) {
                    depth = level;
                    break;
                }
            }
        }
    } else if (action == 'A' && side != 'N') {
        // For add, find where it will be inserted
        if (side == 'B') {
            // For bids: higher prices come first (descending order)
            if (bids.empty()) {
                depth = 0;
            } else {
                int level = 0;
                bool found = false;
                for (auto it = bids.begin(); it != bids.end() && level < 10; ++it, ++level) {
                    if (price > it->first) {
                        // Higher price, insert before this level
                        depth = level;
                        found = true;
                        break;
                    } else if (std::fabs(it->first - price) < 1e-9) {
                        // Same price, same level
                        depth = level;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // Lower than all existing prices
                    depth = level; // Insert at the end of current levels
                }
            }
        } else if (side == 'A') {
            // For asks: lower prices come first (ascending order)
            if (asks.empty()) {
                depth = 0;
            } else {
                int level = 0;
                bool found = false;
                for (auto it = asks.begin(); it != asks.end() && level < 10; ++it, ++level) {
                    if (price < it->first) {
                        // Lower price, insert before this level
                        depth = level;
                        found = true;
                        break;
                    } else if (std::fabs(it->first - price) < 1e-9) {
                        // Same price, same level
                        depth = level;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // Higher than all existing prices
                    depth = level; // Insert at the end of current levels
                }
            }
        }
    }
    
    // Process actions according to business rules
    switch (action) {
        case 'R': // Clear/Reset
            clear();
            break;
            
        case 'A': // Add order
            if (side != 'N') {
                add_order(order_id, price, size, side);
            }
            break;
            
        case 'C': // Cancel order
            if (side != 'N') {
                cancel_order(order_id, size);
            }
            break;
            
        case 'T': // Trade - don't affect orderbook but generate output
            // No orderbook changes, but we still generate output
            break;
            
        case 'F': // Fill - ignore completely, don't generate output
            output_line = "";
            return;
            
        default:
            output_line = "";
            return;
    }
    
    // COMPANY REQUIREMENT: Include all actions (no filtering at orderbook level)
    // Company filtering happens at input processing level (T->F->C sequences)
    
    bool should_generate_output = true; // Include all actions by default
    
    if (should_generate_output) {
        output_line = get_mbp_10_snapshot(fields, 0, depth);
    } else {
        output_line = ""; // Skip actions not significantly impacting market depth
    }
}
