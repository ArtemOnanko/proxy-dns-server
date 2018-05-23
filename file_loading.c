#include "dns_header.h"

FILE*  init;

// 4 last bytes for redirecting ip addr
char response[] = {0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x58, 0x00, 0x04, 0, 0, 0, 0};  

int load_init(void)
{
    node* insertSymbol(node* current, int i) ;
    node* createNewNode(void);
    
    int i,j = 0, counter = 4;
    char xxx[3];    
    init = fopen("server.txt", "r");
    
    // checking if file opens
    if (init == NULL) 
    {
        return FALSE;
    }
    else
    {
        for(i = 0; servaddr[i] = fgetc(init), servaddr[i] != '\n'; i++)     // parse servaddr
        {
        }
        servaddr[i] = '\0';
        for(i = 0; upservaddr[i] = fgetc(init), upservaddr[i] != '\n'; i++) // parse upservaddr
        {
        }
        upservaddr[i] = '\0';
        for(i = 0; localaddr[i] = fgetc(init), localaddr[i] != '\n'; i++)   // parse localaddr and insert it to response[]
        {   
            if(localaddr[i] != '.')
            {
                xxx[j++] = localaddr[i];
            }
            else
            {
                response[RESPONSE_SIZE - counter] = atoi(xxx);
                counter--;
                for(j = 0; j < sizeof(xxx); j++)
                {
                    xxx[j] = 0;
                }
                j = 0;
            }   
        }
        response[RESPONSE_SIZE - counter] = atoi(xxx);
        localaddr[i] = '\0';
    
        printf("Proxy server IP is %s\nUpper level dns server IP is %s\nIP address for redirecting %s\n", servaddr, upservaddr, localaddr); 
    
        // start loading structure into memory
        root = createNewNode();
    
        // defining current node pointing to root
        node* current = root;
        
        // index for children[] (up to ALLOWED_SIGN value)
        int index = 0; 
        char word[PACKET_SIZE];
        char letter = 0;
        for(;;) 
        {
            //getting strings from init file one by one
            fgets(word, PACKET_SIZE, init);
            for (i = 0; word[i] != 10; i++) // till the eol
            {
                letter = word[i];
                if(letter >= 'A' && letter <= 'Z')            // upper case letters
                {
                    letter = letter + ('a' - 'A');            // make lower case
                    index = letter - 'a';  
                }
                else if (letter >= 'a' && letter <= 'z')      // lower case letters
                {
                    index = letter - 'a';                     // index 0 --> 'a', index 1 --> 'b' ...
                }
                else if (letter >= '0' && letter <= '9')      // numbers
                {
                    index = letter +ALPHABET_LENGTH - '0';
                }
                else if (letter == '.')                       // dot
                {
                    index = ALLOWED_SIGNS - 2;  
                }
                else if (letter == '/')                       // slash
                {
                    index = ALLOWED_SIGNS - 1;
                }
                else
                {
                    printf("The word contains unresolved symbols and would be skipped\n");
                    current = root;
                    break;
                }
                current = insertSymbol(current, index);
            }
            
            current -> is_word = TRUE;
            current = root;
            
            if (feof(init))   // till the eof
                break;
        }
        fclose(init);
        return TRUE;
    }
}

void create_redirect_answer(char buf[], int numbytes)
{
    buf[2] = buf[2]|0x84;      // answer, authoritative
    buf[3] = 0x00;             // error 5 (or 0 for local address)
    buf[7] = 1;                // one respond
   
    memcpy(&buf[numbytes], response, RESPONSE_SIZE);
    if ((numbytes_send = sendto(sockfd, buf, (numbytes_recv+RESPONSE_SIZE), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) 
    {
        perror("talker: sendto");
        exit(1);
    }        
    
}

node* createNewNode(void) 
{
    node* newNode = malloc(sizeof (node));
    newNode -> is_word = 0;
    for (int i = 0; i < ALLOWED_SIGNS; i++) 
    {
        newNode -> children[i] = NULL;
    }
    return newNode;
}

// function for symbol "insertion" to the trie
node* insertSymbol(node* current, int i) 
{
    if (current -> children[i] == NULL) 
    {
        node* newNode = createNewNode();
        current -> children[i] = newNode;
        current = newNode;
    } 
    else 
    {
        current = current -> children[i];
    }
    return current;
}

// unloads structure from memory.  Returns true if successful else false.
int unload(void) 
{
    void freeNode(node* node) 
    {
        for (int i = 0; i < ALLOWED_SIGNS; i++) 
        {
            // if node's children point to somewhere
            if (node -> children[i] != NULL) 
            {
                // recursevely calling next level node and checking it's children
                freeNode(node -> children[i]);
            }
        }
        // if node's children point to nowhere - clearing this node
        free(node);
    }
    // should free all malloced nodes plus root node
    freeNode(root);
    return TRUE;
}

// print out whole structure
void print_trie(node* node, int depth, char buf[]) 
{
    for (int i = 0; i < ALLOWED_SIGNS; i++) 
    {
        if (node -> children[i] != NULL) 
        {
            if (i == (ALLOWED_SIGNS - 2))           // dot
                buf[depth] = '.';
            else if (i == (ALLOWED_SIGNS - 1))      // slash
                buf[depth] = '/';
            else if (i > ALPHABET_LENGTH)
                buf[depth] = i - ALPHABET_LENGTH + '0';
            else                                    // letters 
                buf[depth] = i + 'a';
            print_trie(node -> children[i], depth + 1, buf);
        } 
        else
            buf[depth] = '\0';
    }
    if (node -> is_word == TRUE)
        printf("%s\n", buf);
}

