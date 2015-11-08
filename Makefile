include defs.mak

APP_NAME = wiimedia

SOURCES = wiimedia.c libmpdclient.c

CFLAGS += -I../libcwiid
LDFLAGS += -L../libcwiid
LDLIBS += -lcwiid
INST_DIR = ${exec_prefix}/bin

include $(COMMON)/include/app.mak

distclean: clean
	rm Makefile

.PHONY: distclean
