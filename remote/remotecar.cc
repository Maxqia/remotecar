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
#include <cmath>

#include "../packets.h"
#define BUFSIZE 1024

#define MIN_PWM 700.0
#define MAX_PWM 1023.0
#define AXIS_CONV (1023.0 / (2.0 * JoystickEvent::MAX_AXES_VALUE))

#define LEFTAXIS_CORRECTION 1024
#define LEFTAXIS_DEADZONE 8000

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

double convAxisPWM(double axis, int maxSize, int deadZone) {
    double ret = 0;
    if (abs(axis) > deadZone) {
        int negative = axis / abs(axis);
        ret = axis;
        ret -= negative * deadZone; // remove the deadzone
        
        // same as below, but handle negative values instead of the 2 * MAX_AXES_VALUE
        //ret = (ret * (MAX_PWM - MIN_PWM / maxSize)) + negative * MIN_PWM;
        ret = (ret * ((MAX_PWM - MIN_PWM) / maxSize)) + negative * MIN_PWM;
    }
    return ret;
}

int main(int argc, char** argv)
{
  //char buf[BUFSIZE];
  printf("%i\n", sizeof(struct ControlPacket));
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
    
    // set read to no delay so we don't get stuck there
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
  // -- end socket setup --
  
  static int leftTrigger = 0, rightTrigger = 0, leftStick = 0;
  while (true) {
    // Attempt to sample an event from the joystick
    JoystickEvent event;
    while (joystick.sample(&event)) {
      if (event.isAxis()) {
        bool left = false;
        //printf("Axis %u is at position %d\n", event.number, event.value);
        switch (event.number) {
            case 2:
                leftTrigger = event.value + JoystickEvent::MAX_AXES_VALUE; // make them only positive
                break;
            case 5:
                rightTrigger = event.value + JoystickEvent::MAX_AXES_VALUE;
                break;
            case 0:
                leftStick = event.value + LEFTAXIS_CORRECTION;
                break;
         }      
      }
    }
    
    for (int i=0; i<200; i++) {
       printf("\b");
    }

    double turnPWM = convAxisPWM(leftStick, JoystickEvent::MAX_AXES_VALUE, LEFTAXIS_DEADZONE);
    /*if (abs(leftStick) > LEFTAXIS_DEADZONE) {
        int negative = leftStick / abs(leftStick);
        turnPWM = leftStick;
        turnPWM -= negative * LEFTAXIS_DEADZONE; // remove the deadzone
        
        // same as below, but handle negative values instead of the 2 * MAX_AXES_VALUE
        turnPWM = (turnPWM * (MAX_PWM - MIN_PWM / JoystickEvent::MAX_AXES_VALUE)) + negative * MIN_PWM;
    }*/

    double bothPWM = convAxisPWM(rightTrigger - leftTrigger, 2 * JoystickEvent::MAX_AXES_VALUE, 0);
    //bothPWM = (bothPWM * ((MAX_PWM - MIN_PWM) / (2 * JoystickEvent::MAX_AXES_VALUE))) + MIN_PWM;
    struct ControlPacket control;
    control.pwm[0] = bothPWM + turnPWM;
    control.pwm[1] = bothPWM - turnPWM;

    printf("turnPWM : %f, rightTrigger : %i, leftTrigger : %i, bothPWM : %f\n", turnPWM, rightTrigger, leftTrigger, bothPWM);
    
    static struct StatusPacket info;

    /* send the message line to the server */
    n = sendto(sockfd, &control, sizeof(struct ControlPacket), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
    if (n < 0) 
    error("ERROR writing to socket");
    
    // Restrict rate
    usleep(100000); // 100 ms

    /* print the server's reply */
    //bzero(buf, BUFSIZE);
    //n = read(sockfd, buf, BUFSIZE);
    struct StatusPacket newPacket;
    socklen_t src_addr_len = sizeof(serveraddr);
    n = recvfrom(sockfd, &newPacket, sizeof(struct StatusPacket), 0, (struct sockaddr *) &serveraddr, &src_addr_len);
    if (n == sizeof(struct StatusPacket)) {
        info = newPacket;
    }
	
    printf("Left PWM : %.4i; Right PWM : %.4i; Left Wheel Speed : %.4i; Right Wheel Speed : %.4i\n",
           control.pwm[0], control.pwm[1], info.intcounts[0], info.intcounts[1]);
  }
  close(sockfd);
  return 0;
}
