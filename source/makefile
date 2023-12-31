CC=gcc

IDIR+=. ../Toy/source
CFLAGS+=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-ltoy -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2 -lcurses

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))

REPLLIBS = $(wildcard ../Toy/repl/lib*) $(wildcard ../Toy/repl/repl_tools.*) $(wildcard ../Toy/repl/drive_system.*)

OUTNAME=box

ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	LIBLINE=-Wl,-rpath,. -Wl,--out-implib=../$(BOX_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=../$(BOX_OUTDIR)/$(OUTNAME).dll
else ifeq ($(shell uname),Linux)
	LIBLINE=-Wl,-rpath,. -Wl,--out-implib=../$(BOX_OUTDIR)/lib$(OUTNAME).a -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=../$(BOX_OUTDIR)/lib$(OUTNAME).so
	CFLAGS += -fPIC
else ifeq ($(OS),Windows_NT)
	LIBLINE=-Wl,-rpath,. -Wl,--out-implib=../$(BOX_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=../$(BOX_OUTDIR)/$(OUTNAME).dll
else
	@echo "Platform test failed - what platform is this?"
	exit 1
endif

library: repllibs $(OBJ)
	$(CC) -DBOX_EXPORT $(CFLAGS) -shared -o $(OUT) $(LIBLINE) -L../$(BOX_OUTDIR) $(LIBS)

static: repllibs $(OBJ)
	ar crs ../$(BOX_OUTDIR)/lib$(OUTNAME).a $(OBJ)

library-release: $(OBJ) library
	strip $(OUT)

static-release: $(OBJ) static
	strip -d ../$(BOX_OUTDIR)/lib$(OUTNAME).a

#copy the stuff from Toy/repl that is needed
repllibs:
	cp $(REPLLIBS) .

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)

clean-repllibs:
	$(RM) $(notdir $(REPLLIBS))