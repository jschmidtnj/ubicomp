all: README.md
	pandoc README.md --pdf-engine=xelatex -o writeup.pdf

week1: week1.md
	pandoc week1.md --pdf-engine=xelatex -o week1.pdf

week2: week2.md
	pandoc week2.md --pdf-engine=xelatex -o week2.pdf

week2.zip: week2 code/week2.cpp code/week2_carriers.h
	zip -r week2.zip week2.pdf code/week2.cpp code/week2_carriers.h
