AM_CFLAGS  = -D LOCALEDIR=\"$(prefix)/share/locale\" -D EBOOK_SPEAKER \
             -Wall -Wextra -Wunused -Wlogical-op -g \
             -I /usr/include/libxml2/ -O2 -fstack-protector-all \
             -Wstack-protector --param=ssp-buffer-size=4 \
             -Wuninitialized -Winit-self -fno-common \
             -D PROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"
AM_LDFLAGS = -l ncursesw -l sox -l xml2 -l magic
bin_PROGRAMS = eBook-speaker
eBook_speaker_SOURCES = daisy.h daisy3.c eBook-speaker.c list_dir.c
