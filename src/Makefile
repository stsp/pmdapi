# pmdapi, built with dj64dev as a plain 32bit DOS program (dj32).
DJTGT ?= dj32
SOURCES = startup.c wrapper.c msdoshlp.c dpmi_api.c \
    msdos/msdos.c msdos/segreg.c msdos/msdos_ldt.c msdos/instr_dec.c \
    msdos/callbacks.c msdos/emm.c msdos/xms.c msdos/lio.c msdos/hlpmisc.c
AS_SOURCES = entry.S
TGT = pmdapi
EXE = pmdapi.exe

# the below vars must be named exactly like this for dj64.mk
CFLAGS = -Wall -O2 -ggdb3 -fms-extensions -fplan9-extensions -I. -Imsdos
DJCFLAGS = $(CFLAGS)
OBJECTS = $(SOURCES:.c=.o)
AS_OBJECTS = $(AS_SOURCES:.S=.o)

all: $(EXE)

clean: clean_$(DJTGT)
	$(RM) $(TGT) $(EXE) $(EXE).dbg $(OBJECTS)

install: $(EXE)
	cp $(EXE) ..

DJMK = $(shell pkg-config --silence-errors --variable=makeinc $(DJTGT))
ifeq ($(wildcard $(DJMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error $(DJTGT) not installed)
endif
clean_$(DJTGT):
else
include $(DJMK)
endif

$(OBJECTS): $(wildcard *.h msdos/*.h)

$(EXE): $(DJ64_XLIB)
	$(LINK) -d $@.dbg $(DJ64_LINKARGS) $< -o $@ $(DJ64_XLDFLAGS)
