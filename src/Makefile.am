AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -D LOCALEDIR=\"$(prefix)/share/locale\" -D EBOOK_SPEAKER \
            -Wall -Wextra -g \
            -I /usr/include/libxml2/ -O3 --param=ssp-buffer-size=4 \
            -Wunused-function -Wuninitialized -Winit-self -fno-common \
            -D PROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"

bin_PROGRAMS = eBook-speaker

eBook_speaker_SOURCES = daisy.h list_dir.c \
                        common.c daisy2.02.c daisy3.c eBook-speaker.c

localedir = $(datadir)/locale
