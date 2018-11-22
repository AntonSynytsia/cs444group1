COMPILER = gcc
#CFLAGS = -Wall -fpic -coverage -lm -std=c99
CFLAGS = -Wall -lpthread -mrdrnd

export PATH := /usr/local/apps/tex_live/current/bin/x86_64-linux/:$(PATH)

LATEX	= latex -shell-escape
BIBTEX	= bibtex
DVIPS	= dvips
DVIPDF	= dvipdft
XDVI	= xdvi -gamma 4
GH		= gv

EXAMPLES = $(wildcard *.c)
SRC := $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG = $(SRC:%.tex=%.dvi)
PSF = $(SRC:%.tex=%.ps)
PDF = $(SRC:%.tex=%.pdf)

pdf: $(PDF)

ps: $(PSF)

$(TRG): %.dvi: %.tex $(EXAMPLES)
	$(LATEX) $<
	#$(BIBTEX) $(<:%.tex=%)
	$(LATEX) $<
	$(LATEX) $<


$(PSF):%.ps: %.dvi
	$(DVIPS) -R -Poutline -t letter $< -o $@

$(PDF): %.pdf: %.ps
	ps2pdf $<
	cp $(PDF) ../Result/

show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done

showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

concurrency3: concurrency3.c
	$(COMPILER) -o concurrency3 concurrency3.c $(CFLAGS)

concurrency3_2: concurrency3_2.c
	$(COMPILER) -o concurrency3_2 concurrency3_2.c $(CFLAGS)

all: concurrency3 concurrency3_2 pdf

clean:
	rm -f concurrency3.exe concurrency3 *.o *.gcov *.gcda
	rm -f concurrency3_2.exe concurrency3_2 *.o *.gcov *.gcda
	rm -f *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg *.toc
	rm -rf _minted*

.PHONY: all show clean ps pdf showps
