#

CC = gcc
DEFINES =  
CFLAGS=-O2 -g -I. -I.. -Wall -Werror $(DEFINES)

include ../config.mak

DEFINES += -DVERSION=\"$(VERSION)\" 

PROGS = mpegprobe mpprobe mpegvextract mpextract
 
all: $(PROGS)

# targets

mpprobe: mpprobe.c ../libMPEG.a
	@echo building $@ ...
	@$(CC) $(CFLAGS) -o $@ mpprobe.c ../libMPEG.a

mpextract: mpextract.c ../libMPEG.a
	@echo building $@ ...
	@$(CC) $(CFLAGS) -o $@ mpextract.c ../libMPEG.a

mpegprobe: mpegprobe.c ../libMPEG.a
	@echo building $@ ...
	@$(CC) $(CFLAGS) -o $@ mpegprobe.c ../libMPEG.a

mpegvextract: mpegvextract.c ../libMPEG.a
	@echo building $@ ...
	@$(CC) $(CFLAGS) -o $@ mpegvextract.c ../libMPEG.a

# utilities

clean:
	@echo cleaning tools ...
	@rm -f $(PROGS) core*
