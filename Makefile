CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -Wextra -Wno-write-strings -pthread
LDFLAGS = -flto -pthread

SRCS = simple_server.cpp http_server.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp http_server.hh
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean