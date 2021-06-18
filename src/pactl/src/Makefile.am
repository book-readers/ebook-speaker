AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -I . -D LOCALEDIR=\"$(prefix)/share/locale\"  -Wall -Wextra \
            -Wunused -Wunused-variable -Wunused-function \
            -Wall -Wextra -Wconversion -Wmissing-declarations \
            -ggdb -I /usr/include/libxml2/ -O3 \
            -Wuninitialized -Winit-self \
            -DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"

DEPENDENCIES = $(depend)

LDADD = $(ldadd)

CLEANFILES = $(EXTRA_PROGRAMS)

libtool:
	$(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

depend = @LIBOBJS@

profile:
	pactl-set-cmd

.c.s:
	$(COMPILE) -S $<

again:
	$(MAKE) clean
	$(MAKE)

.PHONY:
	profile again
