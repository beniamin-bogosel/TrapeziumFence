PDF = main.pdf

.PHONY: all clean

all: $(PDF)

$(PDF): main.tex figures/current_survivor_rectangles.png
	pdflatex -interaction=nonstopmode main.tex
	pdflatex -interaction=nonstopmode main.tex

clean:
	rm -f main.aux main.log main.out main.toc $(PDF)
