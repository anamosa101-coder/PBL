# Makefile for Multi-Process Simulator (Week 2)
# POSIX Message Queue implementation: Logger -> Core -> UI

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -lrt

TARGETS = logger core ui

.PHONY: all clean run

all: $(TARGETS)

logger: logger.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

core: core.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ui: ui.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o cpu_log.txt error_log.txt
	@echo "Cleaned build artifacts and log files."
