CC = i586-pc-msdosdjgpp-gcc
LD = i586-pc-msdosdjgpp-gcc
CFLAGS = -Wall -fms-extensions -fplan9-extensions
CPPFLAGS = -I.
CFILES = wrapper.c emm.c instremu.c msdoshlp.c startup.c handlers.c calls.c \
    msdos/msdos.c msdos/segreg.c msdos/msdos_ldt.c msdos/instr_dec.c \
    msdos/callbacks.c
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

$(OFILES): $(wildcard *.h)
$(PROGRAM): $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $(OFILES)
