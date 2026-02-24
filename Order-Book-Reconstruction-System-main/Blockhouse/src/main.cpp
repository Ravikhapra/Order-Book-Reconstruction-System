#include "../include/orderbook.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <sstream>

int main(int argc, char* argv[]) {
    // Performance optimization
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_mbo_file>" << std::endl;
        return 1;
    }
    
    std::string input_filename = argv[1];
    std::string output_filename = "output.csv";
    
    // Open input file
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        std::cerr << "Error: Cannot open input file " << input_filename << std::endl;
        return 1;
    }
    
    // Open output file
    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) {
        std::cerr << "Error: Cannot create output file " << output_filename << std::endl;
        return 1;
    }
    
    // Write CSV header
    output_file << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence,";
    output_file << "bid_px_00,bid_sz_00,bid_ct_00,ask_px_00,ask_sz_00,ask_ct_00,";
    output_file << "bid_px_01,bid_sz_01,bid_ct_01,ask_px_01,ask_sz_01,ask_ct_01,";
    output_file << "bid_px_02,bid_sz_02,bid_ct_02,ask_px_02,ask_sz_02,ask_ct_02,";
    output_file << "bid_px_03,bid_sz_03,bid_ct_03,ask_px_03,ask_sz_03,ask_ct_03,";
    output_file << "bid_px_04,bid_sz_04,bid_ct_04,ask_px_04,ask_sz_04,ask_ct_04,";
    output_file << "bid_px_05,bid_sz_05,bid_ct_05,ask_px_05,ask_sz_05,ask_ct_05,";
    output_file << "bid_px_06,bid_sz_06,bid_ct_06,ask_px_06,ask_sz_06,ask_ct_06,";
    output_file << "bid_px_07,bid_sz_07,bid_ct_07,ask_px_07,ask_sz_07,ask_ct_07,";
    output_file << "bid_px_08,bid_sz_08,bid_ct_08,ask_px_08,ask_sz_08,ask_ct_08,";
    output_file << "bid_px_09,bid_sz_09,bid_ct_09,ask_px_09,ask_sz_09,ask_ct_09,";
    output_file << "symbol,order_id\n";
    
    OrderBook orderbook;
    std::vector<std::string> all_lines;
    std::string line;
    bool first_line = true;
    
    // Read all lines first for look-ahead capability
    while (std::getline(input_file, line)) {
        if (first_line) {
            first_line = false;
            continue; // Skip header
        }
        all_lines.push_back(line);
    }
    input_file.close();
    
    uint64_t row_index = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Process each line with T->F->C sequence detection
    for (size_t i = 0; i < all_lines.size(); i++) {
        std::string current_line = all_lines[i];
        std::vector<std::string> current_fields;
        std::stringstream ss(current_line);
        std::string field;
        
        while (std::getline(ss, field, ',')) {
            current_fields.push_back(field);
        }
        
        if (current_fields.size() < 6) continue;
        
        char action = current_fields[5][0];
        
        // COMPANY APPROACH: Enhanced T->F->C sequence detection and filtering
        // Skip Fill actions and redundant Trade actions in T->F->C patterns
        bool skip_this_action = false;
        
        if (action == 'F') {
            skip_this_action = true; // Always skip Fill actions
        }
        
        // Advanced T->F->C pattern detection with multiple lookahead
        if (action == 'T' && i + 1 < all_lines.size()) {
            // Parse next line
            std::vector<std::string> next_fields;
            std::stringstream next_ss(all_lines[i + 1]);
            std::string next_field;
            while (std::getline(next_ss, next_field, ',')) {
                next_fields.push_back(next_field);
            }
            
            // Check if next action is Fill
            if (next_fields.size() >= 6 && next_fields[5][0] == 'F') {
                // Found T->F pattern, check for subsequent Cancel
                for (size_t j = i + 2; j < std::min(i + 5, all_lines.size()); j++) {
                    std::vector<std::string> future_fields;
                    std::stringstream future_ss(all_lines[j]);
                    std::string future_field;
                    while (std::getline(future_ss, future_field, ',')) {
                        future_fields.push_back(future_field);
                    }
                    
                    if (future_fields.size() >= 6 && future_fields[5][0] == 'C') {
                        // Found complete T->F->C sequence, skip the Trade
                        skip_this_action = true;
                        break;
                    } else if (future_fields.size() >= 6 && 
                              (future_fields[5][0] == 'A' || future_fields[5][0] == 'T')) {
                        // Found different action, no Cancel follows
                        break;
                    }
                }
            }
        }
        
        if (skip_this_action) continue;
        
        std::string output_line;
        orderbook.process_mbo_action(current_line, output_line);
        
        // COMPANY REQUIREMENT: Only output when there's a significant change
        if (!output_line.empty()) {
            // Update the row index at the beginning of the line
            size_t first_comma = output_line.find(',');
            if (first_comma != std::string::npos) {
                output_line = std::to_string(row_index) + output_line.substr(first_comma);
            }
            
            // Write with consistent line ending, no trailing spaces
            output_file << output_line << "\n";
            row_index++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    input_file.close();
    output_file.close();
    
    std::cout << "Processing completed successfully!" << std::endl;
    std::cout << "Output written to: " << output_filename << std::endl;
    std::cout << "Processing time: " << duration.count() << " ms" << std::endl;
    
    return 0;
}
