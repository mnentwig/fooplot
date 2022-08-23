 all: fooplotv1a.exe
 # put FLTK root folder here (no need to "make install", just build)
 FLTK := ../fltk-1.3.8
 CFLAGS := -msse3 -O3 -Wall -Wextra -Wno-cast-function-type -pedantic -Wno-unused-parameter -Wfatal-errors -mwindows -static
 LDSTUFF := ${FLTK}/lib/libfltk.a -lole32 -luuid -lcomctl32
 fooplotv1a.exe: 
 # some checks (can delete this)
	@test -d ${FLTK} || echo "${FLTK} folder was not found. Please edit the makefile and provide a pre-compiled (./configure; make) fltk 1.3.8 source directory"
	@test -f ${FLTK}/lib/libfltk.a || echo "failed to locate libfltk.a. Please double-check your ${FLTK} folder was compiled."
# build
# note: currently everything is in .hpp files => need full recompile on any change
	 g++ ${CFLAGS} -DNDEBUG -o fooplotv1a.exe -I. -I${FLTK} fooplot.cpp ${LDSTUFF}

.PHONY: fooplotv1a.exe
