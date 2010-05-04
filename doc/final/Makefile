.PHONY: final.pdf

final.pdf: final.tex final.bib
	pdflatex final
	bibtex final
	pdflatex final
	pdflatex final

.PHONY: clean
clean:
	rm -f *~ *.log *.aux *.bbl *.blg
