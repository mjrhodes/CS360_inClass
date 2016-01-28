lab2:
	g++ -o server server.cpp
dirFile:
	g++ -o dirFile dirFile.cpp
sigint:
	g++ -o sigint sigint.cpp
server:
	g++ -o server server.cpp
	g++ -o download download.cpp
<<<<<<< HEAD
run:
	./server 30000 /Users/michael/Desktop/School/Winter_2016/CS_360/CS360_inClass/stuff
=======
>>>>>>> 6ba9d7d5975b739e7ee17c87249896ef21e53246
clean:
	rm server download
