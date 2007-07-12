all: tutorial

tutorial: tutorial.tex tutorial.bib
	latex tutorial
	bibtex tutorial
	latex tutorial
	latex tutorial
	dvipdf tutorial

clean:
	rm -rf *.bbl *.aux *~ *.dvi *.log *.toc *.blg *.idx
