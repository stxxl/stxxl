all: tutorial

tutorial: tutorial.pdf

# try to use rules from LaTeX.mk (from latex-utils package)
-include LaTeX.mk

ifeq (,$(LU_UTILS))
# if LaTeX.mk is not available, fallback to our own rules

tutorial.pdf: tutorial.tex tutorial.bib
	latex tutorial
	bibtex tutorial
	latex tutorial
	latex tutorial
	dvipdf tutorial

clean:
	rm -rf *.bbl *.aux *~ *.dvi *.log *.toc *.blg *.idx

distclean: clean
	$(RM) tutorial.pdf

.PHONY: clean

else
# automatically generate .pdf versions of the figures, needed by pdflatex

tutorial.pdf: layer_diagram.pdf vector.pdf

clean::
	$(RM) layer_diagram.pdf vector.pdf

endif


spell:
	ispell -d american tutorial.tex

.PHONY: all tutorial spell
