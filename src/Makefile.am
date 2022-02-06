AM_CFLAGS  = -D LOCALEDIR=\"$(prefix)/share/locale\" -D EBOOK_SPEAKER \
             -Wall -Wextra -Wunused -Wlogical-op -g \
             -I /usr/include/libxml2/ -O2 -fstack-protector-all \
             --param=ssp-buffer-size=4 \
             -Wuninitialized -Winit-self -fno-common \
             -D PROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"
AM_LDFLAGS = -l ncursesw -l sox -l xml2 -l magic # -l cdio -l cdio_cdda \
             -l cdio_paranoia
bin_PROGRAMS = eBook-speaker ebook-speaker
eBook_speaker_SOURCES = daisy3.c eBook-speaker.c daisy.h list_dir.c
ebook_speaker_SOURCES = daisy3.c eBook-speaker.c daisy.h list_dir.c
