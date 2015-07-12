CC = i586-pc-msdosdjgpp-gcc
LD = i586-pc-msdosdjgpp-gcc
CFLAGS = -Wall
LDFLAGS = -Wall
CFILES = msdos.c wrapper.c startup.c handlers.c calls.c
SFILES = entry.S
OFILES = $(CFILES:.c=.o) $(SFILES:.S=.o)
GFILES = asm-offsets.h
PROGRAM = pmdapi.exe

define gen-asm-offsets
	(set -e; \
	 echo "#ifndef __ASM_OFFSETS_H__"; \
	 echo "#define __ASM_OFFSETS_H__"; \
	 echo "/*"; \
	 echo " * Generated at `date`, DO NOT MODIFY."; \
	 echo " */"; \
	 echo ""; \
	 sed -ne "/^->/{s:^->\([^ ]*\) [\$$#]*\([^ ]*\) \(.*\):#define \1 \2 /* \3 */:; s:->::; p;}"; \
	 echo ""; \
	 echo "#endif" )
endef

.SUFFIXES: .c .o .s .S .h

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OFILES) $(GFILES)

#install:
#	cp $(PROGRAM) ..

.c.o .S.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.c.s:
	$(CC) $(CFLAGS) -S -o $@ $<

.s.h:
	$(call gen-asm-offsets) < $< > $@

entry.o: asm-offsets.h

$(PROGRAM): $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $(OFILES)
