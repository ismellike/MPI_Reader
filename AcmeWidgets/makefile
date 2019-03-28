P=reportGenerator
O = main.o AcmeReader.o
Flags = -std=c++11 -g -Wall

Project: $(O)
	mpic++ $(Flags) $(O) -o $(P)
	
main.o: main.cpp AcmeReader.cpp
	mpic++ $(Flags) -c main.cpp
	
AcmeReader.o: AcmeReader.h Widget.cpp
	mpic++ $(Flags) -c AcmeReader.cpp

clean:
	rm *.o $(P)

tar:
	mkdir -p dir_$(P)
	cp *.cpp *.h makefile dir_$(P)
	tar -cvzf dir_$(P).tgz dir_$(P)
	-rm -rf dir_$(P)

run:
	mpirun -np 8 reportGenerator Data3000.txt 2007 I
