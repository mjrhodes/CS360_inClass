dirFile:
	g++ -o dirFile dirFile.cpp
sigint:
	g++ -o sigint sigint.cpp
server:
	g++ -o server server.cpp
	g++ -o download download.cpp
run:
	./server 30000 /Users/michael/Desktop/School/Winter_2016/CS_360/CS360_inClass/stuff
clean:
	rm server download
