LATEX_FILE=render.article

all:
<<<<<<< .mine
	rm ${LATEX_FILE}.{toc,aux,log,nav,out,pdf,snm}; pdflatex ${LATEX_FILE}.tex && pdflatex ${LATEX_FILE}.tex

=======
	rm ${LATEX_FILE}.{toc,aux,log,nav,out,pdf,snm,vrb} 2>/dev/null; pdflatex ${LATEX_FILE}.tex && pdflatex ${LATEX_FILE}.tex; rm ${LATEX_FILE}.{toc,aux,log,nav,out,snm,vrb} 2>/dev/null;
>>>>>>> .r102