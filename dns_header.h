/* 
 * File:   dns_header.h
 * Author: artem
 *
 * Created on May 2, 2018, 12:21 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<netinet/udp.h> 
#include<netinet/ip.h> 

#define MYPORT "53"	// the port users will be connecting to
#define PACKET_SIZE 512
#define RESPONSE_SIZE 16
#define DNS_HEADER_LEN 12
#define ALLOWED_SIGNS 38   // small case letters (26), nubmers (10), dot, slash
#define TRUE 1
#define FALSE 0
#define ALPHABET_LENGTH 26

// global variables with ip addresses from init file

char servaddr[16];
char upservaddr[16];
char localaddr[16]; 

// defining node for creating trie structure

typedef struct node 
{
    int is_word;
    struct node* children[ALLOWED_SIGNS];
}
node;

// defining root node
node* root;

void* get_in_addr(struct sockaddr *sa);
int load_init(void);
void dns_setup(struct addrinfo hints, int* sockfd);
void create_redirect_answer( char buf[], int numbytes);
void print_trie(node* node, int depth,  char buf[]);
int unload(void);
int search(char* word);
void create_forward_message(char* dns_request, int request_len, struct sockaddr_storage source, char* serv);