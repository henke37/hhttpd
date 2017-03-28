httpd : handler.o listen.o main.o nativeFile.o reply.o request.o socket.o client.o 
	gcc -o $@ $^ -lc

clean:
	rm *.o

%.o : %.c
	gcc -c -g -o $@ $< -Wchar-subscripts -Wcomment -Wformat -Werror-implicit-function-declaration -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wuninitialized -Wshadow -Wcast-align -Wsign-compare -Werror
