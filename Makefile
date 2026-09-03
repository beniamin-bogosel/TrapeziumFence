# Makefile for the FLINT/arb fence validator.
#
# Detects whether arb is merged into FLINT (FLINT >= 3, header <flint/arb.h>,
# link -lflint) or is a separate legacy library (<arb.h>, link -larb -lflint).

CC       ?= cc
CPPFLAGS ?=
CFLAGS   ?= -O2 -std=c11 -Wall -Wextra
LDFLAGS  ?=
# OMP=auto uses OpenMP when the compiler supports it.  Set OMP=0 for a
# deterministic serial build, or OMP=1 to require OpenMP support.
OMP      ?= auto

ifneq ($(words $(OMP)),1)
  $(error OMP must be exactly one of auto, 0, or 1)
endif
ifeq ($(filter auto 0 1,$(OMP)),)
  $(error OMP must be auto, 0, or 1)
endif

# ---- FLINT/arb layout detection ---------------------------------------------
# Probe A: merged (flint/arb.h, -lflint).  Probe B: legacy (arb.h, -larb -lflint)
PROBE_A := $(shell printf '#include <flint/arb.h>\nint main(void){arb_t x;arb_init(x);arb_clear(x);return 0;}\n' \
             | $(CC) $(CPPFLAGS) $(CFLAGS) -x c - $(LDFLAGS) -o /dev/null \
               -lflint -lmpfr -lgmp 2>/dev/null && echo yes)
ifeq ($(PROBE_A),yes)
  ARB_DEF := -DARB_IN_FLINT
  ARB_LIBS := -lflint -lmpfr -lgmp
else
  ARB_DEF :=
  ARB_LIBS := -larb -lflint -lmpfr -lgmp
endif

ifeq ($(OMP),0)
  OMPFLAG :=
  BUILD_MODE := serial
else
  OMP_OK := $(shell printf 'int main(void){return 0;}\n' \
              | $(CC) $(CPPFLAGS) $(CFLAGS) -x c - -fopenmp -o /dev/null \
                2>/dev/null && echo yes)
  ifeq ($(OMP_OK),yes)
    OMPFLAG := -fopenmp
    BUILD_MODE := openmp
  else ifeq ($(OMP),1)
    $(error OMP=1 requested, but $(CC) does not support -fopenmp)
  else ifeq ($(OMP),auto)
    OMPFLAG :=
    BUILD_MODE := serial
  else
    $(error OMP must be auto, 0, or 1)
  endif
endif

override CPPFLAGS += $(ARB_DEF)
override CFLAGS   += $(OMPFLAG) -MMD -MP
LDLIBS            += $(ARB_LIBS) -lm

CORE_SRC := geom.c series.c functionals.c admissible.c search.c threshold.c cert.c
BUILD_DIR := build/$(BUILD_MODE)
CORE_OBJ := $(addprefix $(BUILD_DIR)/,$(CORE_SRC:.c=.o))
MAIN_OBJ := $(BUILD_DIR)/fence_validate.o
TEST_OBJ := $(BUILD_DIR)/tests/test_trap.o
DEPS := $(CORE_OBJ:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all clean test FORCE
all: fence_validate

fence_validate: FORCE $(MAIN_OBJ) $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MAIN_OBJ) $(CORE_OBJ) $(LDLIBS)

# unit test harness
test_trap: FORCE $(TEST_OBJ) $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_OBJ) $(CORE_OBJ) $(LDLIBS)

test: test_trap fence_validate
	./test_trap
	sh tests/test_cert_cli.sh

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -c -o $@ $<

FORCE:

-include $(DEPS)

clean:
	rm -f *.o tests/*.o fence_validate test_trap
	rm -rf build
