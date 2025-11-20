# Makefile for cse4001_sync
CXX = g++
CXXFLAGS = -std=c++11 -pthread
TARGET = cse4001_sync
SRC = cse4001_sync.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
