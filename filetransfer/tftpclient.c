#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/sendfile.h>

// ./client -r filename
// ./client -w filename

// opcode 1 = read / 2 = write / 3 = data / 4 = ack 
// 5 = error / set 7 as finish

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main( int argc, char *argv[] )
{
    socklen_t len;
	int fd, portno, x;
	struct sockaddr_in server_addr;
	struct hostent *server;
	char buffer[509];
	char *buf;

	memset(buffer, '\0', 509);

	server = gethostbyname("localhost");
	//portno = 61007;
    portno = 54321;

	if( argc < 3 )
	{
		error("Enter right command\nex) ./client -r filename\n");
	}

    // create socket
	fd = socket( AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 )
	{
		error("Creating socket error \n");
	}

    // timeout set.
	struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
        error("setsockopt error");
    } 
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
        error("setsockopt error");
    } 

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);

	memcpy( (void*)& server_addr.sin_addr,
		server -> h_addr_list[0],
		server -> h_length);

    // read!!
	if( strcmp(argv[1],"-r") == 0 )
	{
		int count = 1;

		printf("I got your read request.\n");
		printf("The file you want is %s \n", argv[2]);

		char *file = argv[2];

        // 001filename
		char opcode[3];
		strcpy( opcode, "001");
		char *packet = strcat(opcode,file);

        // send request packet
		x = sendto(fd,packet,512, 0 , 
     		(struct sockaddr *)&server_addr, sizeof(server_addr));
     	if( x < 0 )
     	{
     		error("Error writing to socket");
     	}

		memset(packet, '\0', 512);
		printf("Read request sent\n");

		FILE *f;
        char path[100];
        strcpy( path, "/clientFiles/");
        strcat(path, file);
    	f = fopen(path, "w");

        // receive #1 data packet
		len = sizeof(server_addr);
    	int recvlen = recvfrom(fd, buffer, 512, 0,
    		(struct sockaddr *) &server_addr, &len);
    	if( recvlen < 0 )
    		return 0;

        //Parse data packet #1

        //data packet
    	if( buffer[0] == '0' && buffer[1] == '1' && buffer[2] == '1')
    	{
    		printf("Data packet #%d recived\n",count);
    		buf = buffer + 3;
    		buf[509] = '\0';
    		buf[510] = '\0';
    		buf[511] = '\0';

    		fputs(buf, f);

            //send ack packet
    		char ack[512] = "100";
    		x = sendto(fd,ack,strlen(ack), 0 , 
     			(struct sockaddr *)&server_addr, sizeof(server_addr));
    	}
        // end packet
    	else if( buffer[0] == '1' && buffer[1] == '1' && buffer[2] == '1')
    	{
    		printf("WRITING DONE \n");
	    	fclose(f);
	    	return 0;
    	}
    	else
    	{
    		return 0;
    	}
    	count++;
	    fclose(f);

        // parse #2 packet ~ end
    	while(1)
    	{
            char path[100];
            strcpy( path, "/clientFiles/");
            strcat(path, file);
			f = fopen(path, "a");

			len = sizeof(server_addr);
    		int recvlen = recvfrom(fd, buffer, 512, 0,
    			(struct sockaddr *) &server_addr, &len);
    		if( recvlen < 0 )
    			return 0;
    	   
            // data packet
    		if( buffer[0] == '0' && buffer[1] == '1' && buffer[2] == '1')
    		{
    			printf("Data packet #%d recived\n",count);
    			buf = buffer + 3;
    			buf[509] = '\0';
    			buf[510] = '\0';
    			buf[511] = '\0';

    			fputs(buf, f);

    			char ack[512] = "100";
    			x = sendto(fd,ack,strlen(ack), 0 , 
     				(struct sockaddr *)&server_addr, sizeof(server_addr));
    		}
            // end packet
    		else if( buffer[0] == '1' && buffer[1] == '1' && buffer[2] == '1')
    		{
    			printf("WRITING DONE \n");
	    		fclose(f);
		    	return 0;
	    	}
   		 	else
    		{
    			return 0;
	    	}

	    	count++;
    		fclose(f);
    	}
		
   		printf("File read ( download ) done! \n");
	}

// write!!
	else if( strcmp(argv[1], "-w") == 0 )
	{
		printf("I got your write request.\n");
		printf("The filename you want to upload is %s \n", argv[2]);

		char *file = argv[2];

        // 010filename
		char opcode[512];
		strcpy( opcode, "010");
		char *packet = strcat(opcode,file);

        // send request 
		x = sendto(fd,packet,512, 0 , 
     		(struct sockaddr *)&server_addr, sizeof(server_addr));
     	if( x < 0 )
     	{
     		error("Error writing to socket");
     	}

		memset(packet, '\0', 512);

		printf("WRITE request sent\n");

		int count = 1;
		FILE *f;
       
        char path[100];
        strcpy( path, "/clientFiles/");
        strcat(path, file);
		f = fopen(path, "r");
		if( f == NULL )
		{
			error("file open error");
		}
	
		int n = 1;

		while( n == 1 )
		{
			memset(buffer, '\0', 509);
			n = fread( buffer, 509, 1, f);
			len = sizeof(server_addr);
	    	int recvlen = recvfrom(fd, packet, 512, 0,
   		 		(struct sockaddr *) &server_addr, &len);
	    	
            if( recvlen < 0)
	    		return 0;

            // need to get ack
    		if(packet[0] == '1' && packet[1] == '0' && packet[2] == '0')
    		{
    			printf("get ACK #%d \n", count);
    		}
    		else
    		{
    			error("ACK is not comming. Error\n");
    		}	
			
			memset(opcode, '\0', 3);
			memset(packet, '\0', 512);

			packet[0] = '0';
			packet[1] = '1';
			packet[2] = '1';

			strcat(packet, buffer);

            // send 011data
     		x = sendto(fd,packet,512, 0 , 
     			(struct sockaddr *)&server_addr, sizeof(server_addr));

     		if( x < 0 )
     		{
     			error("Error writing to socket");
     		}
     		else
     		{
     			printf("Data packet #%d sent\n", count);
     		}

     		count++;
     	}	

     	printf("DONE writing!!\n");

     	// end opcode
     	strcpy(opcode, "111");
		packet = strcat(opcode,buffer);
   		x = sendto(fd,packet,512, 0 , 
   			(struct sockaddr *)&server_addr, sizeof(server_addr));
   		printf("END OPCDE SENT \n");
		
		memset(packet, '\0', 512);
	}
	else
	{
		error("Wrong Request! Try -r or -w \n");
	}

	return 1;
}