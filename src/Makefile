CC = i586-pc-msdosdjgpp-gcc
LD = i586-pc-msdosdjgpp-gcc
CFLAGS = -Wall -fms-extensions
LDFLAGS = -Wall
CFILES = msdos.c segreg.c wrapper.c emm.c startup.c handlers.c calls.c
SFILES = entry.S
OFILES = $(CFILES:.c=.o) $(SFILES:.S=.o)
GFILES = asm-offsets.h asm-offsets.s
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

install:
	cp $(PROGRAM) ..

.c.o .S.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c.s:
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $<

asm-offsets.h: asm-offsets.s
	$(call gen-asm-offsets) < $< > $@

entry.o: asm-offsets.h

$(OFILES): $(CFILES) $(wildcard *.h)
$(PROGRAM): $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $(OFILES)
