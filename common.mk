# Set compilers to clang
CC = clang
CXX = clang++
CXXLIB ?= $(CXX) -shared -fPIC

# Is this platform 32 or 64 bit?
ifeq ($(shell uname -m),x86_64)
BITS = 64
else
BITS = 32
endif

# Just in case we port to OSX
SHLIB_SUFFIX ?= so

# Which dirs should be recur into?
DIRS ?=
# What libs does the target depend on?
LIBS ?=
# What directories hold required include files?
INCLUDE_DIRS ?=
# What directories hold require libraries?
LIB_DIRS ?=

# Which make targets should recur into DIRS?
RECURSIVE_TARGETS ?= clean build test

# Match source files, targets, and include files
SRCS ?= $(wildcard *.c) $(wildcard *.cpp)
OBJS32 ?= $(addprefix obj32/, $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SRCS))))
OBJS64 ?= $(addprefix obj64/, $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SRCS))))

# Compatibility include dirs
INCLUDE_DIRS += /usr/include/$(gcc -print-multiarch)

INCLUDES ?= $(wildcard *.h) $(wildcard $(addsuffix /*.h, $(INCLUDE_DIRS)))

INDENT +=" "
export INDENT

LIBFLAGS = $(addprefix -L, $(LIB_DIRS)) $(addprefix -l, $(LIBS))
INCFLAGS = $(addprefix -I, $(INCLUDE_DIRS))

# Set up CFLAGS, if not overridden in Makefile
CFLAGS += -fPIC
CFLAGS32 ?= -m32 $(CFLAGS)
CFLAGS64 ?= -m64 $(CXXFLAGS)

# Set up CXXFLAGS, if not overridden in Makefile
CXXFLAGS ?= $(CFLAGS)
CXXFLAGS32 ?= -m32 $(CXXFLAGS)
CXXFLAGS64 ?= -m64 $(CXXFLAGS)

ifeq ($(BITS),32)
OBJS_NATIVE = $(OBJS32)
CFLAGS_NATIVE = $(CFLAGS32)
CXXFLAGS_NATIVE = $(CXXFLAGS32)
else
OBJS_NATIVE = $(OBJS64)
CFLAGS_NATIVE = $(CFLAGS64)
CXXFLAGS_NATIVE = $(CXXFLAGS64)
endif

# Find all 32-bit, 64-bit, and native shared library targets
LIB_TARGETS32 = $(filter %32.$(SHLIB_SUFFIX), $(TARGETS))
LIB_TARGETS64 = $(filter %64.$(SHLIB_SUFFIX), $(TARGETS))
LIB_TARGETS_NATIVE = $(filter %.$(SHLIB_SUFFIX), $(filter-out %32.$(SHLIB_SUFFIX), $(filter-out %64.$(SHLIB_SUFFIX), $(TARGETS))))

# Any native library targets depend on the explicit-suffix version. Add it to the targets list
ifeq ($(BITS),32)
EXTRA_TARGETS = $(patsubst %.$(SHLIB_SUFFIX), %32.$(SHLIB_SUFFIX), $(LIB_TARGETS_NATIVE))
LIB_TARGETS32 += $(patsubst %.$(SHLIB_SUFFIX), %32.$(SHLIB_SUFFIX), $(LIB_TARGETS_NATIVE))
else
EXTRA_TARGETS = $(patsubst %.$(SHLIB_SUFFIX), %64.$(SHLIB_SUFFIX), $(LIB_TARGETS_NATIVE))
LIB_TARGETS64 += $(patsubst %.$(SHLIB_SUFFIX), %64.$(SHLIB_SUFFIX), $(LIB_TARGETS_NATIVE))
endif

# Find all other targets
OTHER_TARGETS = $(filter-out %.$(SHLIB_SUFFIX), $(TARGETS))

# Build in debug mode by default
all: debug

clean:: 
ifneq ($(TARGETS),)
	@echo $(INDENT)[make] Cleaning build targets
	@rm -f $(TARGETS) $(OBJS64) $(OBJS32) $(EXTRA_TARGETS)
endif

release: DEBUG=
release: build

debug: DEBUG=1
debug: build

test:: build

build:: $(TARGETS) $(INCLUDE_DIRS)

obj32/%.o:: %.c Makefile $(ROOT)/common.mk $(INCLUDE_DIRS) $(INCLUDES)
	@mkdir -p obj32
	@echo $(INDENT)[$(notdir $(firstword $(CC)))] Compiling $< for $(if $(DEBUG),Debug,Release) build
	@$(CC) $(CFLAGS32) $(if $(DEBUG),-g,-DNDEBUG) $(INCFLAGS) -c $< -o $@

obj32/%.o:: %.cpp Makefile $(ROOT)/common.mk $(INCLUDE_DIRS) $(INCLUDES)
	@mkdir -p obj32
	@echo $(INDENT)[$(notdir $(firstword $(CXX)))] Compiling $< for $(if $(DEBUG),Debug,Release) build
	@$(CXX) $(CXXFLAGS32) $(if $(DEBUG),-g,-DNDEBUG) $(INCFLAGS) -c $< -o $@

obj64/%.o:: %.c Makefile $(ROOT)/common.mk $(INCLUDE_DIRS) $(INCLUDES)
	@mkdir -p obj64
	@echo $(INDENT)[$(notdir $(firstword $(CC)))] Compiling $< for $(if $(DEBUG),Debug,Release) build
	@$(CC) $(CFLAGS64) $(if $(DEBUG),-g,-DNDEBUG) $(INCFLAGS) -c $< -o $@

obj64/%.o:: %.cpp Makefile $(ROOT)/common.mk $(INCLUDE_DIRS) $(INCLUDES)
	@mkdir -p obj64
	@echo $(INDENT)[$(notdir $(firstword $(CXX)))] Compiling $< for $(if $(DEBUG),Debug,Release) build
	@$(CXX) $(CXXFLAGS64) $(if $(DEBUG),-g,-DNDEBUG) $(INCFLAGS) -c $< -o $@

$(LIB_TARGETS32):: $(OBJS32) $(INCLUDE_DIRS) $(INCLUDES) Makefile $(ROOT)/common.mk
	@echo $(INDENT)[$(notdir $(firstword $(CXXLIB)))] Linking $@ for $(if $(DEBUG),Debug,Release) build
	@$(CXXLIB) $(CXXFLAGS32) $(INCFLAGS) $(OBJS32) -o $@ $(LIBFLAGS)

$(LIB_TARGETS64):: $(OBJS64) $(INCLUDE_DIRS) $(INCLUDES) Makefile $(ROOT)/common.mk
	@echo $(INDENT)[$(notdir $(firstword $(CXXLIB)))] Linking $@ for $(if $(DEBUG),Debug,Release) build
	@$(CXXLIB) $(CXXFLAGS64) $(INCFLAGS) $(OBJS64) -o $@ $(LIBFLAGS)

$(LIB_TARGETS_NATIVE):: $(patsubst %.$(SHLIB_SUFFIX), %$(BITS).$(SHLIB_SUFFIX), $(LIB_TARGETS_NATIVE))
	@cp $(patsubst %.$(SHLIB_SUFFIX), %$(BITS).$(SHLIB_SUFFIX), $@) $@

$(OTHER_TARGETS):: $(OBJS_NATIVE) $(INCLUDE_DIRS) $(INCLUDES) Makefile $(ROOT)/common.mk
	@echo $(INDENT)[$(notdir $(firstword $(CXX)))] Linking $@ for $(if $(DEBUG),Debug,Release) build
	@$(CXX) $(CXXFLAGS_NATIVE) $(if $(DEBUG),-g,-DNDEBUG) $(INCFLAGS) $(OBJS_NATIVE) -o $@ $(LIBFLAGS)

$(ROOT)/heaplayers:
	git clone ssh://git@github.com/emeryberger/Heap-Layers $(ROOT)/heaplayers

$(RECURSIVE_TARGETS)::
	@for dir in $(DIRS); do \
	  echo "$(INDENT)[$@] Entering $$dir"; \
	  $(MAKE) --no-print-directory -C $$dir $@ DEBUG=$(DEBUG); \
	done
