all: out.pdf
out.pdf: out.cff mkpdf.py
	python mkpdf.py
out.cff: stage1.txt stage2.txt outcff.py
	python outcff.py || rm -f out.cff
clean:
	rm -f out.cff out.pdf gvalue[12].txt stage[12].txt *.o *.bin
