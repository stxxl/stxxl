all: tutorial

tutorial: tutorial.pdf

# try to use rules from LaTeX.mk (from latex-utils package)
-include LaTeX.mk

ifeq (,$(LU_UTILS))
# if LaTeX.mk is not available, fallback to our own rules

tutorial.pdf: tutorial.tex tutorial.bib
	pdflatex tutorial
	bibtex tutorial
	pdflatex tutorial
	pdflatex tutorial

clean:
	rm -rf *.bbl *.aux *~ *.dvi *.log *.toc *.blg *.idx

distclean: clean
	$(RM) tutorial.pdf

.PHONY: clean

endif


spell:
	ispell -d american tutorial.tex

.PHONY: all tutorial spell
