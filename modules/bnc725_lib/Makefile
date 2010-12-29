
MX_ARCH = win32

all: bnc725_lib.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

BNC725_LIB_OBJS = bnc725_lib.$(OBJ) i_bnc725_lib.$(OBJ)

bnc725_lib.mxo: $(BNC725_LIB_OBJS)
	link /dll /debug /nologo /out:bnc725_lib.mxo /nodefaultlib:libc \
		$(BNC725_LIB_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f bnc725_lib.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:bnc725_lib.mxo\;2 \
			-manifest bnc725_lib.mxo.manifest ; \
		rm -f bnc725_lib.mxo.manifest; \
	fi

bnc725_lib.$(OBJ): bnc725_lib.c
	$(COMPILE) $(CFLAGS) bnc725_lib.c

i_bnc725_lib.$(OBJ): i_bnc725_lib.c i_bnc725_lib.h
	$(COMPILE) $(CFLAGS) i_bnc725_lib.c


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

