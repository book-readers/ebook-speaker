AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -D LOCALEDIR=\"$(prefix)/share/locale\" -D EBOOK_SPEAKER \
            -D PROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
            -I /usr/include/libxml2/ \
            -Wall -Wextra -Wconversion -g \
            -Wunused-function -Wuninitialized -Winit-self \
            -Wlogical-op -Wpedantic

bin_PROGRAMS = ebook-speaker

ebook_speaker_SOURCES = daisy.h pactl/src/core-util.c pactl/src/strbuf.c \
                        pactl/src/pactl.c list_dir.c playfile.c \
                        common.c daisy2.02.c daisy3.c ebook-speaker.c \
                        ncx.c opf.c alsa_ctl.c

localedir = $(datadir)/locale

ldadd = $(depend) @LTLIBINTL@

DEPENDENCIES = $(depend)

LDADD = $(ldadd)

CLEANFILES = $(EXTRA_PROGRAMS)

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

.c.s:
	(cd pactl/src && $(MAKE))
	$(COMPILE) -S $<

again:
	$(MAKE) clean
	$(MAKE)
