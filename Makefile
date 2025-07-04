SRCDIR = src
HEADERS = include
BINDIR = bin
USR_HEADERS = /usr/include

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BINDIR)/%.o, $(SRCS))
LIBS = -lssl -lcrypto -ljsoncpp -lcpr

CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++23 -fconcepts-diagnostics-depth=2 -O0 --std=c++23 -fopenmp

TARGET = $(BINDIR)/torrent_cli

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADERS) -I $(USR_HEADERS) -o $@ $(LIBS)

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm -rf $(BINDIR)

.PHONY: all clean
