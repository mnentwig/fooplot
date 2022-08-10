 all: fooplotv1a.exe
 # put FLTK root folder here (no need to "make install", just build)
 FLTK := ../fltk-1.3.8
 CFLAGS := -msse3 -O3 -Wall -Wextra -Wno-cast-function-type -pedantic -Wno-unused-parameter -Wfatal-errors -mwindows -static
 LDSTUFF := ${FLTK}/lib/libfltk.a -lole32 -luuid -lcomctl32
 fooplotv1a.exe:
	 g++ ${CFLAGS} -DNDEBUG -o fooplotv1a.exe -I. -I${FLTK} fooplot.cpp ${LDSTUFF}
# currently everything is in .hpp files => need full recompile on any change
.PHONY: fooplotv1a.exe
