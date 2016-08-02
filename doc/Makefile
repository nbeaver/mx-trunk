
MAN = mxautosave.8 mxautosave.dat.5 mxautosave_fmt.5 mxdriverinfo.1 mxmotor.1 mxserver.8
PDF = $(patsubst %, %.pdf,$(MAN))
HTML = $(patsubst %, %.html,$(MAN))
ASCII = $(patsubst %, %.txt,$(MAN))
all : $(PDF) $(HTML) $(ASCII)

%.pdf : %
	groff -Tpdf -man $< > $@

%.html : %
	groff -Thtml -man $< > $@

%.txt : %
	groff -Tascii -man $< > $@

clean :
	rm -f $(PDF) $(HTML) $(ASCII)

