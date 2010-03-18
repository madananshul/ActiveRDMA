.PHONY: proposal.pdf

proposal.pdf: proposal.tex proposal.bib
	pdflatex proposal
	bibtex proposal
	pdflatex proposal
	pdflatex proposal

.PHONY: clean
clean:
	rm -f *~ *.log *.aux *.bbl *.blg
