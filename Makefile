CXX = g++
CXXFLAGS = -std=c++23 -Wall
TARGET = main
SRC = main.cpp
HEADERS = $(wildcard *.h)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean