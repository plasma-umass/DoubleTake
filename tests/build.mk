DIR              := tests

SIMPLE_TESTS     := simple_leak simple_overflow simple_uaf simple_mt_uaf

TEST_BIN_TARGETS += $(SIMPLE_TARGETS)
TESTS            += simple-tests

SIMPLE_TARGETS   := $(addprefix $(DIR)/, $(addsuffix /simple.test, $(SIMPLE_TESTS)))
SIMPLE_LDFLAGS   += -L. -lpthread

# no optimization - clang is smart enough to see in the 'use after
# free' test that the object is allocated, referenced, freed, and
# used-after-free all in the main function, so it simply optimizes
# _all_ of that away.
%/simple.test: $(CONFIG) $(LIB) $(DIR)/build.mk
	@echo "  LD    $@"
	$(CC) -O0 -std=$(CVER) $(CFLAGS) $(LDFLAGS) $(SIMPLE_LDFLAGS) -MMD -o $@ $(@:simple.test=*.c)

-include $(SIMPLE_TARGETS:.test=.d)

$(SIMPLE_TESTS): $(SIMPLE_TARGETS)
	@echo "  TEST  $@"
	LD_PRELOAD=./libdoubletake.so tests/$@/simple.test
#	LD_LIBRARY_PATH=. tests/$@/simple.test

simple-tests: $(SIMPLE_TESTS)

PHONY_TARGETS += simple-tests $(SIMPLE_TESTS)
