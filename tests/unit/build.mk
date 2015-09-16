DIR              := tests/unit

UNIT_BIN         := $(DIR)/unit.test
TEST_BIN_TARGETS += $(UNIT_BIN)
TESTS            += unit-tests

UNIT_SRCS        := $(wildcard $(DIR)/*.cpp)
UNIT_OBJS        := $(patsubst %.cpp,%.o,$(UNIT_SRCS))

UNIT_LDFLAGS     += -L. -ldttest_s -ldl -lpthread

# -Wextra and -Wundef cause clang to bail out in the gtest headers
GTEST_CXXFLAGS   := $(filter-out -Wextra,$(CXXFLAGS:-Wundef=)) -Wno-unused-const-variable -DGTEST_HAS_PTHREAD=1

$(DIR)/%.o: $(DIR)/%.cpp $(CONFIG) $(DIR)/build.mk
	@echo "  CXX   $@"
	$(CXX) -O0 $(GTEST_CXXFLAGS) -MMD -o $@ -c $<

# no optimizations - clang is smart enough to see in the 'use after
# free' test that the object is allocated, referenced, freed, and
# used-after-free all in the main function, so it simply optimizes
# _all_ of that away.
$(UNIT_BIN): $(CONFIG) $(UNIT_OBJS) $(TESTLIB) $(DIR)/build.mk
	@echo "  LD    $@"
	$(CXX) -O0 $(CFLAGS) -MMD -o $@ $(UNIT_OBJS) $(LDFLAGS) $(UNIT_LDFLAGS)

-include $(UNIT_OBJS:.o=.d)

unit-tests: $(UNIT_BIN)
	./$<

PHONY_TARGETS += simple-tests
