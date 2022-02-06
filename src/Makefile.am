AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -D LOCALEDIR=\"$(prefix)/share/locale\" -D EBOOK_SPEAKER \
            -Wall -Wextra -g \
            -I /usr/include/libxml2/ -O3 --param=ssp-buffer-size=4 \
            -Wunused-function -Wuninitialized -Winit-self -fno-common \
            -D PROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"

bin_PROGRAMS = eBook-speaker

eBook_speaker_SOURCES = daisy.h pactl/src/core-util.c pactl/src/strbuf.c \
                        pactl/src/pactl.c list_dir.c playfile.c \
                        common.c daisy2.02.c daisy3.c eBook-speaker.c

localedir = $(datadir)/locale

ldadd = $(depend) @LTLIBINTL@

DEPENDENCIES = $(depend)

LDADD = $(ldadd)

CLEANFILES = $(EXTRA_PROGRAMS)

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

.c.s:
	(cd pactl/src && make)
	$(COMPILE) -S $<

again:
	$(MAKE) clean
	$(MAKE)
