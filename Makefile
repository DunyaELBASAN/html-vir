
#LATEX_FILE=render.presentation
LATEX_FILE=render.article

all:
	rm ${LATEX_FILE}.{toc,aux,log,nav,out,pdf,snm}; pdflatex ${LATEX_FILE}.tex && pdflatex ${LATEX_FILE}.tex

