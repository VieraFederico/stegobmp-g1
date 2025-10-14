CC           := gcc
PKG_CONFIG   := pkg-config

CHECK_CFLAGS := $(shell $(PKG_CONFIG) --cflags check 2>/dev/null)
CHECK_LIBS   := $(shell $(PKG_CONFIG) --libs   check 2>/dev/null) -pthread

SRCDIR       := src

PROD_SRCS := $(wildcard $(SRCDIR)/*.c) \
             $(wildcard $(SRCDIR)/*/*.c) \
             $(wildcard $(SRCDIR)/*/*/*.c) \
             $(wildcard $(SRCDIR)/*/*/*/*.c)

TEST_SRCS    := $(filter %_test.c,$(PROD_SRCS))
PROD_SRCS    := $(filter-out $(TEST_SRCS),$(PROD_SRCS))

PROD_OBJS    := $(PROD_SRCS:.c=.o)
TEST_OBJS    := $(TEST_SRCS:.c=.o)

MAIN_OBJS    := $(PROD_OBJS)

INCLUDE_DIRS := -I$(SRCDIR) $(addprefix -I,$(wildcard $(SRCDIR)/*/)) $(addprefix -I,$(wildcard $(SRCDIR)/*/*/))

ifeq ($(strip $(DEBUG)),)
  DEBUG_FLAGS :=
else
  DEBUG_FLAGS := -DDEBUG -g
endif

ifeq ($(SANITIZE),1)
  SANITIZE_FLAGS := -fsanitize=address -fsanitize=undefined -fsanitize=leak
  CFLAGS += $(SANITIZE_FLAGS)
  LDFLAGS += $(SANITIZE_FLAGS)
endif

CFLAGS       := -Wall -Wextra -std=gnu99 -O2 $(DEBUG_FLAGS) $(INCLUDE_DIRS) -pthread $(CHECK_CFLAGS)
LDFLAGS      := -lssl -lcrypto -pthread $(CHECK_LIBS)

MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

TARGET       := stegobmp
TEST_TARGET  := test_runner

.PHONY: all test clean check-leaks

all: $(TARGET)

$(TARGET): $(MAIN_OBJS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(MAIN_OBJS) $(TEST_OBJS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(PROD_OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	@echo "Cleaning build artifacts..."
	@find $(SRCDIR) -name '*.o' -delete
	@find $(SRCDIR) -name '*.d' -delete
	@rm -f $(TARGET) $(TEST_TARGET)
	@echo "Clean completed!"

check-leaks: clean
	$(MAKE) SANITIZE=1
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./$(TARGET)