TOPDIR := ../..
include $(TOPDIR)/make/common_vars.mk

PROG := libbase.so
LIBS := -lcgraph -lm

include $(TOPDIR)/make/common_targets.mk

$(PROG): array.c args.c base.c bits.c bitset.c debug.c graphviz.c hash_table.c intset.c list.c ord.c primes.c random.c rbtree.c string.c
	$(CC) $(INCLUDEPATHS) -shared $(CFLAGS) -o $@ $+ $(LIBPATHS) $(LIBS) $(LDFLAGS) -fpic
	mkdir -p ~/lib
	ln -sf "$$(pwd)/$(PROG)" ~/lib/

