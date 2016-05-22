DIR                := tests

SIMPLE_CXX_TESTS   := simple_uaf_cxx
SIMPLE_TESTS       := simple_leak simple_overflow simple_uaf simple_mt_uaf

SIMPLE_TARGETS     := $(addprefix $(DIR)/, $(addsuffix /simple.test, $(SIMPLE_TESTS)))
SIMPLE_CXX_TARGETS := $(addprefix $(DIR)/, $(addsuffix /simple_cxx.test, $(SIMPLE_CXX_TESTS)))
SIMPLE_LDFLAGS     += -L. -lpthread

TEST_BIN_TARGETS   += $(SIMPLE_TARGETS) $(SIMPLE_CXX_TARGETS)
TESTS              += simple-tests

# no optimization - clang is smart enough to see in the 'use after
# free' test that the object is allocated, referenced, freed, and
# used-after-free all in the main function, so it simply optimizes
# _all_ of that away.
%/simple.test: $(CONFIG) $(LIB) $(DIR)/build.mk
	@echo "  LD    $@"
	$(CC) -O0 -std=$(CVER) $(CFLAGS) $(LDFLAGS) $(SIMPLE_LDFLAGS) -MMD -o $@ $(@:simple.test=*.c)

%/simple_cxx.test: $(CONFIG) $(LIB) $(DIR)/build.mk
	@echo "  LD    $@"
	$(CXX) -O0 -std=$(CXXVER) $(CFLAGS) $(LDFLAGS) $(SIMPLE_LDFLAGS) -MMD -o $@ $(@:simple_cxx.test=*.cc)

-include $(SIMPLE_TARGETS:.test=.d)
-include $(SIMPLE_CXX_TARGETS:.test=.d)

$(SIMPLE_TESTS): $(SIMPLE_TARGETS)
	@echo "  TEST  $@"
	LD_PRELOAD=./libdoubletake.so tests/$@/simple.test
#	LD_LIBRARY_PATH=. tests/$@/simple.test

$(SIMPLE_CXX_TESTS): $(SIMPLE_TARGETS)
	@echo "  TEST  $@"
	LD_PRELOAD=./libdoubletake.so tests/$@/simple_cxx.test
#	LD_LIBRARY_PATH=. tests/$@/simple.test

simple-tests: $(SIMPLE_TESTS) $(SIMPLE_CXX_TESTS)

PHONY_TARGETS += simple-tests $(SIMPLE_TESTS) $(SIMPLE_CXX_TESTS)
