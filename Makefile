# Makefile for the FLINT/arb fence validator.
#
# Detects whether arb is merged into FLINT (FLINT >= 3, header <flint/arb.h>,
# link -lflint) or is a separate legacy library (<arb.h>, link -larb -lflint).

CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra
LDFLAGS ?=
# set OMP=0 to build without OpenMP
OMP     ?= 1

# ---- FLINT/arb layout detection ---------------------------------------------
# Probe A: merged (flint/arb.h, -lflint).  Probe B: legacy (arb.h, -larb -lflint)
PROBE_A := $(shell printf '#include <flint/arb.h>\nint main(void){arb_t x;arb_init(x);arb_clear(x);return 0;}\n' \
             | $(CC) -x c - -o /dev/null -lflint -lmpfr -lgmp 2>/dev/null && echo yes)
ifeq ($(PROBE_A),yes)
  ARB_DEF := -DARB_IN_FLINT
  ARB_LIBS := -lflint -lmpfr -lgmp
else
  ARB_DEF :=
  ARB_LIBS := -larb -lflint -lmpfr -lgmp
endif

ifeq ($(OMP),1)
  OMPFLAG := -fopenmp
else
  OMPFLAG :=
endif

CFLAGS  += $(ARB_DEF) $(OMPFLAG)
LDLIBS  := $(ARB_LIBS) -lm

CORE_SRC := geom.c series.c functionals.c admissible.c search.c threshold.c cert.c
CORE_OBJ := $(CORE_SRC:.c=.o)

.PHONY: all clean test
all: fence_validate

fence_validate: fence_validate.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# unit test harness
test_trap: tests/test_trap.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test: test_trap fence_validate
	./test_trap
	sh tests/test_cert_cli.sh

%.o: %.c
	$(CC) $(CFLAGS) -I. -c -o $@ $<

tests/%.o: tests/%.c
	$(CC) $(CFLAGS) -I. -c -o $@ $<

clean:
	rm -f *.o tests/*.o fence_validate test_trap
