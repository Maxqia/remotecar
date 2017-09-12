/* mix of test.cc and tcpclient.c on the internet */
#include "joystick.hh"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> // for TCP_NODELAY
#include <netdb.h> 

#include "../packets.h"
#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char** argv)
{
  //char buf[BUFSIZE];
  printf("%i\n", sizeof(struct StatusPacket));
  //  -- joystick setup --
  // Create an instance of Joystick
  Joystick joystick("/dev/input/js0");

  // Ensure that it was found and that we can use it
  if (!joystick.isFound())
  {
    printf("open failed.\n");
    exit(1);
  }

  //  -- socket setup --
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    //char buf[BUFSIZE];

  //hostname = "192.168.4.207";
  hostname = "192.168.6.170";
  //hostname = "192.168.4.1";
  portno = 23;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    /*if (connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) //edit -- cast to get rid of error
      error("ERROR connecting");*/
  // -- end socket setup --
  
  int leftPWM = 0, rightPWM = 0;
  while (true)
  {
    bool changed = false;
    // Restrict rate
    usleep(100000); // 100 ms

    // Attempt to sample an event from the joystick
    JoystickEvent event;
    while (joystick.sample(&event)) {
      if (event.isAxis()) {
        bool left = false;
        //printf("Axis %u is at position %d\n", event.number, event.value);
        switch (event.number) {
            case 2:
                left = true;
            case 5:
            	long posValue = ((long) event.value + JoystickEvent::MAX_AXES_VALUE);
            	//printf("%li\n", posValue);
            	double normalValue = (posValue * (1023.0 / (2.0 * JoystickEvent::MAX_AXES_VALUE)));
            	//printf("%f\n", normalValue);
                (left ? leftPWM : rightPWM) = normalValue;
                changed = true;
         }      
      }
    }
    
    for (int i=0; i<200; i++) {
       printf("\b");
    }

    struct ControlPacket control;
    control.pwm[0] = leftPWM;
    control.pwm[1] = rightPWM;
    
    struct StatusPacket info;

    /* send the message line to the server */
    n = sendto(sockfd, &control, sizeof(struct ControlPacket), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
    if (n < 0) 
    error("ERROR writing to socket");

    /* print the server's reply */
    //bzero(buf, BUFSIZE);
    //n = read(sockfd, buf, BUFSIZE);
    socklen_t src_addr_len = sizeof(serveraddr);
    n = recvfrom(sockfd, &info, sizeof(struct StatusPacket), 0, (struct sockaddr *) &serveraddr, &src_addr_len);
    if (n < 0) 
    error("ERROR reading from socket");
    //printf("Echo from server: %s\n", buf);
	
    printf("Left PWM : %.4i; Right PWM : %.4i; Left Wheel Speed : %.4i; Right Wheel Speed : %.4i",
           leftPWM, rightPWM, info.intcounts[0], info.intcounts[1]);
  }
  close(sockfd);
  return 0;
}
