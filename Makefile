all:
	gcc -Wall -o dns_server dns_server.c file_loading.c
clean: 
	rm -f dns_server