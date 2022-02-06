AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -I . -D LOCALEDIR=\"$(prefix)/share/locale\"  -Wall -Wextra \
            -Wunused -Wunused-variable -Wunused-function \
            -Wmissing-declarations \
            -ggdb -I /usr/include/libxml2/ -O3 \
            -Wuninitialized -Winit-self \
            -DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"

bin_PROGRAMS = pactl-set-cmd

DEPENDENCIES = $(depend)

LDADD = $(ldadd)

CLEANFILES = $(EXTRA_PROGRAMS)

libtool:
	$(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

pactl_set_cmd_SOURCES = strbuf.c core-util.c pactl.c pactl-set-cmd.c

pactl_set_cmd_DEPENDENCIES = $(DEPENDENCIES)
                        
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
