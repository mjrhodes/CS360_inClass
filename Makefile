dirFile:
	g++ -o dirFile dirFile.cpp
sigint:
	g++ -o sigint sigint.cpp
server:
	g++ -o server server.cpp
	g++ -o download download.cpp
clean:
	rm server download
