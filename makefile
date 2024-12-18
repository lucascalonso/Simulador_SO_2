CXX = /opt/homebrew/bin/g++-14

CXXFLAGS = -std=c++11 $(shell wx-config --cxxflags) -I./include
LDFLAGS = $(shell wx-config --libs)
SRC = $(wildcard src/*.cpp)
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/simulador
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRC)))


all: $(TARGET)


$(TARGET): $(OBJ)
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean