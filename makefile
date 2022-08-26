EXE := fooplotv1a.exe

 all: ${EXE}
 # put FLTK root folder here (no need to "make install", just build)
 FLTK := ../fltk-1.3.8
 
 # -mconsole is needed for console output in cmd.exe (mingw shell works without it)
 # leaving out -msse3 for compatibility
 # -Wno-cast-function-type: FLTK uses one callback function pointer for two different use models
 CFLAGS := -std=c++17 -O3 -Wall -Wextra -Wno-cast-function-type -pedantic -Wno-unused-parameter -Wfatal-errors -mwindows -mconsole -static
 LDSTUFF := ${FLTK}/lib/libfltk.a -lole32 -luuid -lcomctl32
 
 ${EXE}: 
 #=== checks === (only diagnostics info)
	@test -d ${FLTK} || echo "${FLTK} folder was not found. Please edit the makefile and provide a pre-compiled (./configure; make) fltk 1.3.8 source directory"
	@test -f ${FLTK}/lib/libfltk.a || echo "failed to locate libfltk.a. Please double-check your ${FLTK} folder was compiled."
# === build ===
# currently everything is in .hpp files => need full recompile on any change => this is very simple...
	 g++ ${CFLAGS} -DNDEBUG -o ${EXE} -I. -I${FLTK} fooplot.cpp ${LDSTUFF}
	 strip ${EXE}

clean:
	rm -f ${EXE}

# as we have no dependency list, recompile every time
.PHONY: ${EXE} clean
