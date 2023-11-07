all: main

CC = clang
override CFLAGS += -g -Wall 
PROD = -Wextra -Werror  -Wpedantic -Wno-gnu-statement-expression


SRCS = common.c parse.c process.c
MAIN_SRCS = main.c $(SRCS)
TEST_SRCS = test.c $(SRCS)
HEADERS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.h' -print)

main: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(PROD) $(MAIN_SRCS) -o "$@" 

test: $(TEST_SRCS) $(HEADERS)
	echo "make test"
	$(CC) $(CFLAGS) $(TEST_SRCS) -o "$@" 
	$(CC) $(CFLAGS) $(PROD) $(MAIN_SRCS) -o "test_data/main" 
	./test
	diff test_data/mess.txt test_data/tested/mess.txt || exit -1
	diff test_data/process_1_mess.txt test_data/tested/process_1_mess.txt || exit -1
	diff test_data/process_2_mess.txt test_data/tested/process_2_mess.txt || exit -1
	diff test_data/process_chain_mess_out.txt test_data/tested/process_chain_mess_out.txt || exit -1
	echo " =========== main ============ "
	./test_data/main
	git diff --exit-code test_data/ || exit -1
	echo "tests OK"

clean:
	rm -f main test