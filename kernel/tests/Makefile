CURRENT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

TEST_OBJS+=$(CURRENT_DIR)/memory/malloc_tests.o \
		   $(CURRENT_DIR)/memory/get_free_pages_tests.o \
		   $(CURRENT_DIR)/fs/vfs_tests.o \
		   $(CURRENT_DIR)/drivers/bdev/ram/ramdisk_tests.o \
		   $(CURRENT_DIR)/kernel_lib/lru_tests.o
