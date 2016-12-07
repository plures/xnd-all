
# ==============================================================================
#                          Unix Makefile for libndtypes
# ==============================================================================

LIBSTATIC = libndtypes.a

CC ?= gcc
LD ?= gcc
AR ?= ar
RANLIB ?= ranlib
CFLAGS ?= -Wall -Wextra -std=c11 -pedantic -O2 -g

ifeq ($(MAKECMDGOALS), coverage)
  CFLAGS = -O0 -g -fno-inline -fprofile-arcs -ftest-coverage -fpic
  LDFLAGS = -fprofile-arcs
endif


default: $(LIBSTATIC)


OBJS = alloc.o display.o grammar.o lexer.o map.o ndtypes.o parsefuncs.o \
       parser.o seq.o

$(LIBSTATIC):\
Makefile $(OBJS)
	$(AR) rc $(LIBSTATIC) $(OBJS)
	$(RANLIB) $(LIBSTATIC)

alloc.o:\
Makefile alloc.c ndtypes.h
	$(CC) $(CFLAGS) -c alloc.c

display.o:\
Makefile display.c ndtypes.h
	$(CC) $(CFLAGS) -c display.c

grammar.o:\
Makefile grammar.c lexer.h ndtypes.h seq.h grammar.h
	$(CC) $(CFLAGS) -c grammar.c

lexer.o:\
Makefile lexer.c lexer.h grammar.h
	$(CC) $(CFLAGS) -c lexer.c

map.o:\
Makefile map.c ndtypes.h
	$(CC) $(CFLAGS) -c map.c

ndtypes.o:\
Makefile ndtypes.c ndtypes.h
	$(CC) $(CFLAGS) -c ndtypes.c

parsefuncs.o:\
Makefile parsefuncs.c ndtypes.h seq.h parsefuncs.h
	$(CC) $(CFLAGS) -c parsefuncs.c

parser.o:\
Makefile parser.c ndtypes.h
	$(CC) $(CFLAGS) -c parser.c

seq.o:\
Makefile seq.c ndtypes.h seq.h
	$(CC) $(CFLAGS) -c seq.c

# Flex generated files
lexer.h:\
Makefile lexer.c

lexer.c:\
Makefile lexer.l ndtypes.h parsefuncs.h
	flex -o lexer.c --header-file=lexer.h lexer.l

# Bison generated files
grammar.h:\
Makefile grammar.c

grammar.c:\
Makefile grammar.y lexer.h
	bison -Wall -o grammar.c --defines=grammar.h grammar.y


# Tests
runtest:\
Makefile $(LIBSTATIC) FORCE
	$(CC) -I. $(CFLAGS) -o tests/runtest tests/runtest.c \
	    tests/alloc_fail.c tests/test_parse.c tests/test_parse_error.c \
            tests/test_parse_roundtrip.c tests/test_nominal.c $(LIBSTATIC) 

check:\
Makefile runtest
	./tests/runtest

# Tests with injected allocation failures
runtest_alloc:\
Makefile $(LIBSTATIC) FORCE
	$(CC) -I. $(CFLAGS) -DTEST_ALLOC -o tests/runtest tests/runtest.c \
	    tests/alloc_fail.c tests/test_parse.c tests/test_parse_error.c \
            tests/test_parse_roundtrip.c tests/test_nominal.c $(LIBSTATIC)

memcheck:\
Makefile runtest_alloc
	valgrind --leak-check=full --show-leak-kinds=all --suppressions=tests/valgrind.supp ./tests/runtest

# Coverage
coverage:\
Makefile clean runtest_alloc
	./tests/runtest
	for file in *.c; do gcov -l "$$file" > /dev/null 2>&1; done

# Benchmark
bench:\
Makefile bench.c $(LIBSTATIC)
	$(CC) $(CFLAGS) -o bench bench.c $(LIBSTATIC)

# Parse a file that contains a datashape type
indent:\
Makefile indent.c parser.c ndtypes.h $(LIBSTATIC)
	$(CC) $(CFLAGS) -o indent indent.c $(LIBSTATIC)


clean: FORCE
	rm -f *.o *.gcov *.gcda *.gcno bench indent tests/runtest $(LIBSTATIC)


FORCE:


