export CFLAGS+=-std=c18 -pedantic -Werror

export BOX_OUTDIR = out
export TOY_OUTDIR = ../out

all: $(BOX_OUTDIR) library

#lib builds
library: $(BOX_OUTDIR) toy-library
	$(MAKE) -j8 -C source library

static: $(BOX_OUTDIR) toy-static
	$(MAKE) -j8 -C source static

library-release: clean $(BOX_OUTDIR) toy-library-release
	$(MAKE) -j8 -C source library-release

static-release: clean $(BOX_OUTDIR) toy-static-release
	$(MAKE) -j8 -C source static-release

#toy builds
toy-library:
	$(MAKE) -j8 -C Toy/source library

toy-static:
	$(MAKE) -j8 -C Toy/source static

toy-library-release:
	$(MAKE) -j8 -C Toy/source library-release

toy-static-release:
	$(MAKE) -j8 -C Toy/source static-release

#distribution
dist: export CFLAGS+=-O2 -mtune=native -march=native
dist: library-release

#utils
$(BOX_OUTDIR):
	mkdir $(BOX_OUTDIR)

#utils
.PHONY: clean

clean:
ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	find . -empty -type d -delete
else ifeq ($(shell uname),Linux)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	rm -rf out
	find . -empty -type d -delete
else ifeq ($(OS),Windows_NT)
	$(RM) *.o *.a *.exe 
else ifeq ($(shell uname),Darwin)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.dylib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	rm -rf out
	find . -empty -type d -delete
else
	@echo "Deletion failed - what platform is this?"
endif

rebuild: clean all
