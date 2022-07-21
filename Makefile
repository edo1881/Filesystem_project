CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes 
LIBS= 
CC=gcc
AR=ar


BINS= shell

OBJS=linenoise.o\
	 disk_driver.o\
	 linked_list.o\
	 fat32.o
#add here your object files

HEADERS=linenoise.h\
		disk_driver.h\
		linked_list.h\
	    fat32.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

shell: shell.c $(OBJS) 
	$(CC) $(CCOPTS)  -o $@ $^ -lm

clean:
	rm -rf *.o *~  $(BINS)
