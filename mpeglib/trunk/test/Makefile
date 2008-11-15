#

CC = gcc
DEFINES = 
CFLAGS=-O2 -g -I. -I.. -Wall -Werror $(DEFINES)

PROGS = extract_mpeg2
 
all: $(PROGS)

# targets

extract_mpeg2: extract_mpeg2.c
	@echo building $@ ...
	@$(CC) $(CFLAGS) -o $@ $<

# utilities

clean:
	@echo cleaning tests and helpers ...
	@rm -f $(PROGS) core*
