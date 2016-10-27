
MX_ARCH = win32

BNC_DIR = c:\\opt\\bnc

all: bnc725.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(BNC_DIR)

BNC725_OBJS = bnc725.$(OBJ) i_bnc725.$(OBJ) d_bnc725.$(OBJ)

bnc725.mxo: $(BNC725_OBJS)
	link /dll /debug /nologo /out:bnc725.mxo /nodefaultlib:libc \
		$(BNC725_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(BNC_DIR)\\BNC725.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f bnc725.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:bnc725.mxo\;2 \
			-manifest bnc725.mxo.manifest ; \
		rm -f bnc725.mxo.manifest; \
	fi

bnc725.$(OBJ): bnc725.c i_bnc725.h d_bnc725.h
	$(COMPILE) $(CFLAGS) bnc725.c

i_bnc725.$(OBJ): i_bnc725.cpp i_bnc725.h
	$(COMPILE) $(CFLAGS) i_bnc725.cpp

d_bnc725.$(OBJ): d_bnc725.cpp d_bnc725.h i_bnc725.h
	$(COMPILE) $(CFLAGS) d_bnc725.cpp


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

