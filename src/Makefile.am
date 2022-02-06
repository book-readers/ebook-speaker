AM_CFLAGS = -D PREFIX=\"/usr/local/\" -D EBOOK_SPEAKER -Wall -Wextra -Wunused -g -I /usr/include/libxml2/ -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -Wuninitialized -DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"
AM_LDFLAGS = -lncursesw -lsox -lxml2 -lmagic # -lcdio -lcdio_cdda -lcdio_paranoia -lmagic
bin_PROGRAMS = eBook-speaker
eBook_speaker_SOURCES = daisy3.c eBook-speaker.c daisy.h list_dir.c
