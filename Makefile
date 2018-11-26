# Makefile:
# AMAN ANAND - 1481975
# CMPUT - 379 Assignment#3
# ------------------------------------------------------------

target=		    submit
allFiles=       Makefile a3sdn.cpp controller.cpp switch.cpp AssignmentReport_3.pdf
# ------------------------------------------------------------
all: starter

starter:  starter.c
	g++ -std=c++11 a3sdn.cpp -o a3sdn.out

starter.c:

tar:
	touch $(target).tar
	tar -cvf $(target).tar $(allFiles)


clean:
	rm -rf *.out

