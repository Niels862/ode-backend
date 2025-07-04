TARGET = obc
CC = g++
INC_DIR = inc
SRC_DIR = src
CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -std=c++17 -O3 -g

INCFLAGS = $(addprefix -I, $(INC_DIR))
SOURCES = $(sort $(shell find $(SRC_DIR) -name '*.cpp'))
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCFLAGS) -MMD -o $@ -c $<

test: $(TARGET)
	@python3 scripts/test.py obc tests

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)
	
-include $(DEPS)
