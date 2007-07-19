all: tutorial

tutorial: tutorial.pdf

tutorial.pdf: tutorial.tex tutorial.bib
	latex tutorial
	bibtex tutorial
	latex tutorial
	latex tutorial
	dvipdf tutorial

spell:
	ispell -d american tutorial.tex

clean:
	rm -rf *.bbl *.aux *~ *.dvi *.log *.toc *.blg *.idx

.PHONY: all tutorial spell clean
