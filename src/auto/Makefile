TOPDIR := ../..
include $(TOPDIR)/make/common_vars.mk

PROG := auto
LIBS += -lcgraph -lregex

include $(TOPDIR)/make/common_targets.mk

$(PROG): main.c dot.c print_ast.c
	$(CC) $(INCLUDEPATHS) $(CFLAGS) -o $@ $+ $(LIBPATHS) $(LIBS) $(LDFLAGS)
