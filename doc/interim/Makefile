.PHONY: interim.pdf

interim.pdf: interim.tex interim.bib
	pdflatex interim
	bibtex interim
	pdflatex interim
	pdflatex interim

.PHONY: clean
clean:
	rm -f *~ *.log *.aux *.bbl *.blg
