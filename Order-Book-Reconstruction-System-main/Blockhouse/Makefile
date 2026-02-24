CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -pipe -march=native
LDFLAGS =

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = .

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))
EXECUTABLE = $(BINDIR)/reconstruction

.PHONY: all clean test

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(EXECUTABLE) output.csv

test: $(EXECUTABLE)
	./$(EXECUTABLE) data/mbo.csv
	@echo "Comparing output with expected result..."
	@if diff -q output.csv data/mbp.csv > /dev/null 2>&1; then \
		echo "SUCCESS: Output matches expected result!"; \
	else \
		echo "DIFFERENCE: Output differs from expected result"; \
		echo "First 10 differences:"; \
		diff output.csv data/mbp.csv | head -20; \
	fi

benchmark: $(EXECUTABLE)
	@echo "Running performance benchmark..."
	@time ./$(EXECUTABLE) data/mbo.csv
