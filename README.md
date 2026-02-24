The Order Book Reconstruction System is a high-performance C++ application designed to reconstruct a full order book from raw Market-By-Order (MBO) data and convert it into Market-By-Price (MBP-10) format.

This system processes order-level events such as order additions, cancellations, modifications, and executions to rebuild the real-time market depth. It aggregates orders at each price level and maintains accurate bid and ask information using efficient data structures.

This project simulates how real financial exchanges manage and maintain order books.

Features

Reconstructs order book from raw market data

Converts Market-By-Order (MBO) to Market-By-Price (MBP-10)

Supports bid and ask order processing

Handles order addition, cancellation, and execution

Maintains price-time priority

High-performance and efficient processing

Clean and modular C++ implementation

Technologies Used

C++

STL (Standard Template Library)

Maps

Hash Tables

File Handling

Project Structure
Order-Book-Reconstruction-System/
│
├── src/                # Source code files
├── data/               # Input data files
├── output/             # Generated output files
├── README.md          # Project documentation
How It Works

The system performs the following steps:

Reads raw order event data

Processes order events such as:

New order

Cancel order

Modify order

Trade execution

Maintains bid and ask order book

Aggregates orders based on price

Generates Market-By-Price output

Input

Market-By-Order (MBO) data file containing:

Order ID

Price

Quantity

Order Type

Timestamp

Output

Market-By-Price (MBP-10) format showing:

Top 10 Bid prices

Top 10 Ask prices

Quantity at each price level

Installation
Step 1: Clone the repository
git clone https://github.com/RAHUKKRRANJAN/Order-Book-Reconstruction-System.git
Step 2: Navigate to project folder
cd Order-Book-Reconstruction-System
Step 3: Compile the program
g++ main.cpp -o orderbook
Step 4: Run the program
./orderbook
Applications

Algorithmic Trading

Financial Market Analysis

Trading Strategy Backtesting

Market Simulation

Educational Learning

Performance

Efficient processing using STL data structures

Handles large volume market data

Fast and scalable system

Future Improvements

Real-time data streaming support

Visualization dashboard

Multi-threading support

Web-based interface
