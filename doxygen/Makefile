
all: doxygen

doxygen: Doxyfile
	doxygen Doxyfile
	( cd output/latex; make )

clean:
	-rm -rf output

distclean: clean

