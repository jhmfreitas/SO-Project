# Makefile, versao 3
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CC       = gcc
CFLAGS   = -g -std=gnu99 -Wall -pedantic -pthread

.PHONY: all clean zip

all: heatSim_p4

heatSim_p4: p4_main.o matrix2d.o util.o
	$(CC) $(CFLAGS) -o $@ $+

p4_main.o: p4_main.c matrix2d.h util.h
	$(CC) $(CFLAGS) -o $@ -c $<

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -o $@ -c $<

util.o: util.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o heatSim_p3

run:
	./heatSim_p4 5 10.0 10.0 0.0 0.0 10 5 0.1 saveFile 2

zip: heatSim_p4_solucao.zip

heatSim_p4_solucao.zip: Makefile p4_main.c matrix2d.h util.h matrix2d.c matrix2d.h util.c
	zip $@ $+

