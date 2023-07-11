################################################################################

CC		= gcc
CFLAGS		= -march=native -O3 -funroll-loops -fpic
REASON		= @if [ -f $@ ]; then echo "[$@: $?]"; else echo "[$@]"; fi

all: solver-x-pt

################################################################################

norm: normalize.c
	$(REASON)
	$(CC) -fopenmp $(CFLAGS) -o $@ $^

perm: permute.c
	$(REASON)
	$(CC) $(CFLAGS) -o $@ $^

################################################################################

sudoku-100000000: perm
	perm 20 < sudoku-5000000-normal > /tmp/sudoku
	shuffle /tmp/sudoku > $$
	$(RM) /tmp/sudoku

sudoku-1000000: perm
	head -n 1000000 sudoku-5000000-normal | perm 1 > $@

################################################################################

solver-a-omp: solver-a.c sudoku-1000000
	$(REASON)
	$(CC) -DOPENMP -fopenmp -fprofile-generate $(CFLAGS) -o $@ $^ 
	$@ < sudoku-1000000 > /dev/null
	$(CC) -DOPENMP -fopenmp -fprofile-use $(CFLAGS) -o $@ $^
	strip $@

solver-a-pt: solver-a.c sudoku-1000000
	$(REASON)
	$(CC) -DPTHREAD -pthread -fprofile-generate $(CFLAGS) -o $@ $^ 
	$@ < sudoku-1000000 > /dev/null
	$(CC) -DPTHREAD -pthread -fprofile-use $(CFLAGS) -o $@ $^
	strip $@

solver-a: solver-a.c
	$(REASON)
	$(CC) -DOPENMP $(CFLAGS) -o $@ $^
	strip $@

################################################################################

solver-x-omp: solver-x.c sudoku-1000000
	$(REASON)
	$(CC) -DOPENMP -fopenmp -fprofile-generate $(CFLAGS) -o $@ $^ 
	$@ < sudoku-1000000 > /dev/null
	$(CC) -DOPENMP -fopenmp -fprofile-use $(CFLAGS) -o $@ $^
	strip $@

solver-x-pt: solver-x.c sudoku-1000000
	$(REASON)
	$(CC) -DPTHREAD -pthread -fprofile-generate $(CFLAGS) -o $@ $^ 
	$@ < sudoku-1000000 > /dev/null
	$(CC) -DPTHREAD -pthread -fprofile-use $(CFLAGS) -o $@ $^
	strip $@

solver-x: solver-x.c
	$(REASON)
	$(CC) -DOPENMP $(CFLAGS) -o $@ $^
	strip $@

################################################################################

tags:
	ctags --format=2 -o $@ *.h *.cc

deps depend: *.h *.c
	$(REASON)
	$(CC) -MM $(CPPFLAGS) *.c > deps

clean:
	$(RM) *.o *.gcda 

distclean: clean
	$(RM) core log deps tags *~ solver-a-omp solver-a-pt

-include deps

################################################################################
