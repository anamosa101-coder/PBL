# Makefile for CPU Process Scheduling Simulator (Week 3)

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c99
TARGET = scheduler
SRCS = scheduler.c algorithms.c gantt.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run compare benchmark1 benchmark2 benchmark3

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

compare: $(TARGET)
	./$(TARGET) --compare

benchmark1: $(TARGET)
	./$(TARGET) --benchmark 1

benchmark2: $(TARGET)
	./$(TARGET) --benchmark 2

benchmark3: $(TARGET)
	./$(TARGET) --benchmark 3

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJS)
	@echo "Cleaned build artifacts."
