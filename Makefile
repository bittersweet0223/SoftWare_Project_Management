CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread
LDFLAGS :=

TARGET := chat
SRCS := main.cpp tcp_chat.cpp des_utils.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
