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

#define MYPORT "53"	// the port users will be connecting to
#define PACKET_SIZE 512
#define RESPONSE_SIZE 16
#define DNS_HEADER_LEN 12



char servaddr[16];
char upservaddr[16];
char localaddr[16]; 

void load_init(void);
void dns_setup(struct addrinfo hints, int* sockfd);
void create_redirect_answer(unsigned char buf[], int numbytes);