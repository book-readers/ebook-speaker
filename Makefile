CC      = gcc
CFLAGS  = -Wall -g
PREFIX  = /usr/local/
LD      = $(CC)
LDLIBS  = -lncursesw -lidn -lsox -lzip
OFILES  = eBook-speaker.o

all: $(OFILES)
	$(LD) $(OFILES) $(LDLIBS) -o eBook-speaker

eBook-speaker.o: eBook-speaker.c
	$(CC) $(CFLAGS) -D PREFIX=\"$(PREFIX)\" -c $< -o $@

clean:
	rm -f *.o eBook-speaker

install:        
	@./install.sh $(PREFIX)                        
