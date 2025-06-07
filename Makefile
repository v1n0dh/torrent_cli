SRCDIR = src
HEADERS = include
BINDIR = bin

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BINDIR)/%.o, $(SRCS))
LIBS = -lssl -lcrypto -ljsoncpp -lcpr

CXX = g++
CXXFLAGS = -Wall -Wextra -pg -std=c++23 -fconcepts-diagnostics-depth=2 -O0 --std=c++23 -fopenmp -fsanitize=address

TARGET = $(BINDIR)/torrent_cli

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADERS) -o $@ $(LIBS)

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm -rf $(BINDIR)

.PHONY: all clean
