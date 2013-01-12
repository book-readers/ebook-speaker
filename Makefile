CC      = gcc
CFLAGS  = -g -Wall -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Wformat-security -Werror=format-security -I /usr/include/libxml2/
PREFIX  = /usr/local/
LD      = $(CC)
LDLIBS  = -lncursesw -lsox -lmagic -lxml2
OFILES  = eBook-speaker.o daisy3.o # beep.o
             
all: $(OFILES)
	$(LD) $(OFILES) $(LDLIBS) -o eBook-speaker

eBook-speaker.o: eBook-speaker.c
	$(CC) $(CFLAGS) -D PREFIX=\"$(PREFIX)\" -c $< -o $@

daisy3.o: daisy3.c
	$(CC) $(CFLAGS) -D PREFIX=\"$(PREFIX)\" -c $< -o $@

clean:
	rm -f *.o eBook-speaker eBook-speaker.1 eBook-speaker.html

install:
	@./install.sh $(PREFIX)

beep.o: beep.c
	$(CC) $(CFLAGS) -D PREFIX=\"$(PREFIX)\" -c $< -o $@
