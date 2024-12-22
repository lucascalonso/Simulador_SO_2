#CXX = /opt/homebrew/bin/g++-14

#Usar compilador correto
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
else
    CXX = g++
endif

CXXFLAGS = -std=c++11 -I./include
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