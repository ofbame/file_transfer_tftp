#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/sendfile.h>
#include <math.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    socklen_t len;
	int fd, portno, x;
    struct timeval tv;
	struct sockaddr_in server_addr, client_addr;
	char packet[512];
	char buffer[512];
	char *buf;

    memset(buffer, '\0', 512);
    memset(packet, '\0', 512);

	//portno = 61007;
    portno = 12345;

    // create socket
	fd = socket( AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 )
	{
		printf("Creating socket error \n");
		exit(0);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portno);

    // binding
	if( bind( fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		error("Binding error \n");
	}

    // loop for start from begining after timeout.
    LOOP: printf("start of the Loop\n");

    // set timeout
    tv.tv_sec = INFINITY;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
        goto LOOP;
    } 
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
        goto LOOP;
    }
    
    memset(buffer, '\0', 512);
    memset(packet, '\0', 512);
    len = sizeof(client_addr);

    // get request.
    x = recvfrom(fd, packet, 512, 0,
    	(struct sockaddr *) &client_addr, &len);
    if( x < 0)
    	return 0;
	printf("request recived\n");
	
	//read
	if( packet[0] == '0' && packet[1] == '0' && packet[2] == '1')
	{
		char *file;
        file = packet+3;

		printf("I got read request! \n");
		printf("send the file to client : %s\n", file);

		FILE *f;
 		char path[100];
        strcpy( path, "/serverFiles/");
        strcat(path, file);

    	f= fopen(path, "r");

    	if( f == NULL )
		{
			error("file open error");
		}

		int count = 1;

		char rbuf[509];
		int n = 1;
		while( n == 1 )
		{
			memset(rbuf, '\0', 509);
			n = fread( rbuf, 509, 1, f );

			memset(packet, '\0', 512);

			packet[0] = '0';
			packet[1] = '1';
			packet[2] = '1';

			strcat(packet, rbuf);

            //send #1 data packet
			x = sendto(fd,packet,512, 0 , 
     			(struct sockaddr *)&client_addr, sizeof(client_addr));
			if( x < 0)
				return 0;

            //timeout... 2sec
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
                goto LOOP;
            } 
            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
                goto LOOP;
            }

			x = recvfrom(fd, packet, 512, 0,
    			(struct sockaddr *) &client_addr, &len);		

			if(packet[0] == '1' && packet[1] == '0' && packet[2] == '0')
    		{
    			printf("get ACK #%d \n", count);
    		}
    		else
    		{
    			printf("ACK is not comming. Error\n");
                goto LOOP;
    		}
            count++;
		}

     	printf("DONE sending to client!!\n");
		packet[0] = '1';
		packet[1] = '1';
		packet[2] = '1';

     	x = sendto(fd,packet,512, 0 , 
     			(struct sockaddr *)&client_addr, sizeof(client_addr));
		if( x < 0)
			return 0;
	}

	//write
	else if( packet[0] == '0' && packet[1] == '1' && packet[2] == '0')
	{
		printf("I got write request! \n");
		int count = 1;
		char ack[512] = "100";

		x = sendto(fd,ack,strlen(ack), 0 , 
   	 			(struct sockaddr *)&client_addr, sizeof(client_addr));
    	printf("ACK #%d sent \n",count);
   		if( x < 0 )
	 	{
    		printf("Error writing to socket");
   			exit(0);
  		}

    	printf("packet : %s\n",packet);
        char *filename;
        filename =packet+3;

	    FILE *f;
        char path[100];
        strcpy( path, "/serverFiles/");
        strcat(path, filename);
    	f= fopen(path, "w");
    	len = sizeof(client_addr);
        
        //timeout
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
            goto LOOP;
        } 
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
            goto LOOP;
        }

    	x = recvfrom(fd, buffer, 512, 0,
    		(struct sockaddr *) &client_addr, &len);
    	if( x < 0)
    		return 0;
    	if( buffer[0] == '0' && buffer[1] == '1' && buffer[2] == '1')
    	{
    		printf("Data packet #%d recived\n",count);
    		buf = buffer + 3;
    		buf[509] = '\0';
    		buf[510] = '\0';
    		buf[511] = '\0';

    		fputs(buf, f);
    	}
    	else if( buffer[0] == '1' && buffer[1] == '1' && buffer[2] == '1')
    	{
    		printf("WRITING DONE \n");
	    	fclose(f);
    	}
    	else
    	{
    		printf("??");
    		return 0;
    	}

    	count++;
   		fclose(f);

		while(1)
		{
			char ack[512] = "100";

			x = sendto(fd,ack,strlen(ack), 0 , 
    	 			(struct sockaddr *)&client_addr, sizeof(client_addr));
	    	printf("ACK #%d sent \n",count);
    		if( x < 0 )
   		 	{
	    		printf("Error writing to socket");
    			exit(0);
    		}
	    	char *filename;
    		filename =packet+3;

		    FILE *f;
            char path[100];
            strcpy( path, "/serverFiles/");
            strcat(path, filename);
    		f = fopen(path, "a");
 
            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
               goto LOOP;
            } 
            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0) {
                goto LOOP;
            }

    		len = sizeof(client_addr);
    		x = recvfrom(fd, buffer, 512, 0,
    			(struct sockaddr *) &client_addr, &len);
    		if( x < 0)
    			return 0;

            //parse packet
    		if( buffer[0] == '0' && buffer[1] == '1' && buffer[2] == '1')
    		{
    			printf("Data packet #%d recived\n",count);
    			buf = buffer + 3;
    			buf[509] = '\0';
	    		buf[510] = '\0';
    			buf[511] = '\0';

    			fputs(buf, f);

    		}
    		else if( buffer[0] == '1' && buffer[1] == '1' && buffer[2] == '1')
    		{
    			printf("WRITING DONE \n");
	    		fclose(f);
	    		break;
    		}
    		else
    		{
    			break;
    		}
    		fclose(f);
    		count++;
    	}
    }
	else if( packet[0] == '1' && packet[1] == '0' && packet[2] == '1')
	{
		error("error detected!");
	}
	else
	{
		error("opcode error");
	}

    close(fd);
    return 0;
}