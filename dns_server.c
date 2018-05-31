/* 
 * File:   dns_server.c
 * Author: artem.onanko@gmail.com
 *
 * Created on April 4, 2018, 2:40 PM
 */

#include "dns_header.h"
 
int main(void)
{      
    int sockfd, numbytes_recv, numbytes_send, labellen, i = 0;
    struct addrinfo hints;
    struct sockaddr_storage their_addr;
    char buf[PACKET_SIZE+4];
    char* parsed_name = malloc(PACKET_SIZE);
    char* ptr_parsing;
    char s[INET_ADDRSTRLEN];                 
    socklen_t addr_len = sizeof(struct sockaddr);
        
    // handler for the interrupt signal (to clean up)
    void sigint_handler(int sig)
    {
        free(parsed_name);
        unload();
        close(sockfd);
        exit(EXIT_SUCCESS);
    }
        
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
            
    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
                       
    // load ip addresses, and trie structure from init file
    int loaded = load_init();
    if(loaded == FALSE)
    {
        printf("Couldn't open init file and load structure!\n");
        exit(EXIT_FAILURE);
    }
        
    // print out the whole structure
    print_trie(root, 0, buf); 
             
    // setting up our proxy server for receiving datagrams on port 53
    dns_setup(hints, &sockfd);
                 
    // starting iterative udp server
    for(;;)                                 
    {
        printf("proxy server: waiting to recvfrom...\n");
        numbytes_recv = recvfrom(sockfd, buf, PACKET_SIZE + 4, 0, (struct sockaddr *)&their_addr, &addr_len);
        printf("Received %i bytes of data from %s\n", numbytes_recv, inet_ntop(AF_INET, get_in_addr((struct sockaddr *)&their_addr), s, INET_ADDRSTRLEN));
                                  
        // Obtain addr from dns request in a simple way
        {
            ptr_parsing = buf + DNS_HEADER_LEN;
            
            while ((labellen = *ptr_parsing++))
            {
                while (labellen--)
                {
                    parsed_name[i++] = *ptr_parsing++;
                }
                parsed_name[i++] = '.';
            }
            parsed_name[--i] = 0;
            i = 0;
            printf("%s\n", parsed_name);
        }               
                                         
        // Here should be the check if addr is in black list 	            
        int blacklist = search(parsed_name);
            
        if(!blacklist)
        {
            // forwarding message to dns upserver
            create_forward_message(buf, numbytes_recv, their_addr, upservaddr);
        }
        else
        {
            // host is not resolved  
            create_redirect_answer(buf, numbytes_recv);  
                                
            if ((numbytes_send = sendto(sockfd, buf, (numbytes_recv+RESPONSE_SIZE), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) 
            {
                perror("proxy server: sendto");
                exit(EXIT_FAILURE);
            }       
            printf("proxy server: sent %d bytes to %s\n", numbytes_send, inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
        }
    }
    return 0;
}

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

void dns_setup(struct addrinfo hints, int* sockfd)
{
    struct addrinfo *servinfo, *p;
    int rv;
    int yes = 1;            // for loosing addr already in use message
       
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;  
        
    if ((rv = getaddrinfo(servaddr, MYPORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	exit(EXIT_FAILURE);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("proxy server: socket");
            continue;
        }
        // loose the "Address already in use" message
        if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
        {
            perror("setsockopt");
            continue;
        }
        if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(*sockfd);
            perror("proxy server: bind");
            continue;
        }
        break;
    }	
    
    if (p == NULL) 
    {
        fprintf(stderr, "proxy server: failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);
}

int search(char* word) 
{
    node* current = root;
    int index = 0;
    int i = 0;
    char c = 0;
    int word_length = strlen(word);
    
    for (i = 0; i < word_length; i++) 
    {
        c = word[i];
        if (c >= 'a' && c <= 'z')              // letters
            index = c - 'a';
        else if (c >= '0' && c <= '9')         // numbers
            index = c + ALPHABET_LENGTH -'0';
        else if (c == '.')                     // dot
            index = ALLOWED_SIGNS - 2;
        else if (c == '/')                      // slash
            index = ALLOWED_SIGNS - 1;
        else                                    // unresolved symbols    
        {
            printf("No such word in the file\n");
            return FALSE;
        }                                   
          
        if (current -> children[index] == NULL) 
        {
            printf("No such word in the file\n");
            return FALSE;
        } 
        else
            current = current -> children[index];
    }
    if (current -> is_word == TRUE) 
    {
        printf("There is such word in the file\n");
        return TRUE;                    // there is such word in the file
    }       
    else 
    {
        printf("No such word in the file\n");
        return FALSE;
    }
}

 // from https://www.binarytides.com/raw-udp-sockets-c-linux/
void create_forward_message(char* dns_request, int request_len, struct sockaddr_storage source, char* serv)   
{
    char str[INET_ADDRSTRLEN];  
    struct sockaddr* source_addr;
    source_addr = (struct sockaddr*)&source;
     
    // 96 bit (12 bytes) pseudo header needed for udp header checksum calculation 
    struct pseudo_header
    {
        u_int32_t source_address;
        u_int32_t dest_address;
        u_int8_t placeholder;
        u_int8_t protocol;
        u_int16_t udp_length;
    };
    
    //Generic checksum calculation function
    unsigned short csum(unsigned short *ptr,int nbytes) 
    {
        register long sum;
        unsigned short oddbyte;
        register short answer;
 
        sum = 0;
        while(nbytes>1) 
        {
            sum+=*ptr++;
            nbytes-=2;
        }
        if(nbytes==1) 
        {
            oddbyte=0;
            *((u_char*)&oddbyte)=*(u_char*)ptr;
            sum+=oddbyte;
        }
 
        sum = (sum>>16)+(sum & 0xffff);
        sum = sum + (sum>>16);
        answer=(short)~sum;
     
        return(answer);
    }
    
    //Create a raw socket of type IPPROTO
    int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
     
    if(s == -1)
    {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create raw socket");
        exit(EXIT_FAILURE);
    }
     
    //Datagram to represent the packet
    char datagram[4096] , source_ip[32] , *data , *pseudogram;
     
    //zero out the packet buffer
    memset (datagram, 0, 4096);
     
    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;
     
    //UDP header
    struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct iphdr));
    struct sockaddr_in sin;
    struct pseudo_header psh;
     
    //Data part
    data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
    memcpy(data , dns_request, request_len);
     
    //some address resolution
    strncpy(source_ip , inet_ntop(source.ss_family, get_in_addr(source_addr), str, sizeof str),sizeof(source_ip));
    
    sin.sin_family = AF_INET;
    sin.sin_port = htons(53);
    sin.sin_addr.s_addr = inet_addr ( serv );
     
    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + request_len;
    iph->id = htonl (54321); //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;
     
    //Ip checksum
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);
     
    //UDP header
    udph->source = ((struct sockaddr_in*)source_addr)->sin_port;
    udph->dest = htons (53);
    udph->len = htons(8 + request_len); //udp header size
    udph->check = 0; //leave checksum 0 now, filled later by pseudo header
     
    //Now the UDP checksum using the pseudo header
    psh.source_address = inet_addr( source_ip );
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr) + request_len );
     
    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + request_len;
    pseudogram = malloc(psize);
    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + request_len);
     
    udph->check = csum( (unsigned short*) pseudogram , psize);
     
    //Send the packet
    if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
    //Data send successfully
    else
    {
        printf ("Packet Send. Length : %d \n" , iph->tot_len);
    }
    free(pseudogram);
}
