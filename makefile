# Defina o compilador  /opt/homebrew/bin/g++-14
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
else
    CXX = g++
endif

# Flags de compilação e linkagem
CXXFLAGS = -std=c++11 -stdlib=libc++ `wx-config --cxxflags` -I./include
LDFLAGS = -stdlib=libc++ `wx-config --libs`
SRC = $(wildcard src/*.cpp)
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/simulador
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRC)))
DEP = $(OBJ:.o=.d)

$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MF $(OBJDIR)/$*.d -c $< -o $@

-include $(DEP)

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(BINDIR)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean