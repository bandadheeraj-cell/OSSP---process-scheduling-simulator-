CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -Iinclude -pthread -g
LDFLAGS = -pthread

SRCS = src/pssim_main.c \
       src/pssim_shell.c \
       src/pssim_parser.c \
       src/pssim_proc.c \
       src/pssim_builtins.c \
       src/pssim_signals.c \
       src/pssim_pipes.c \
       src/pssim_redirect.c \
       src/pssim_scheduler.c \
       src/pssim_threads.c \
       src/pssim_memory.c

TEST_SRCS = tests/test_pssim.c \
            src/pssim_scheduler.c \
            src/pssim_threads.c \
            src/pssim_memory.c \
            src/pssim_parser.c

OBJS = $(SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)

TARGET = ps_shell_sim
TEST_TARGET = test_pssim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) $(LDFLAGS)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --batch workloads/sample_workload_1.txt 2

clean:
	rm -f src/*.o tests/*.o $(TARGET) $(TEST_TARGET)

.PHONY: all clean test valgrind
