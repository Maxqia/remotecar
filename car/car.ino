#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const int leftWheel = 4;
const int rightWheel = 5;
const int statusLED = 16;

#include "utils.hpp"
#include "control.hpp"

#define CMDBUFFERSIZE 50
WiFiUDP Udp;

void setup() {
  pinMode(statusLED, OUTPUT);
  digitalWrite(statusLED, LOW);
  
  Serial.begin(115200);
  connectWifi();
  setupOTA();

  // Start the server
  Udp.begin(23);
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  digitalWrite(statusLED, HIGH);

  yield();

  Serial.println("test");
  pinMode(leftWheel, OUTPUT);
  pinMode(rightWheel, OUTPUT);
  //analogWriteFreq(128); // 2 for testing
  Serial.println("Start!");
  
  Serial.print("PWM Range : 0 - ");
  Serial.println(PWMRANGE);
  setupControl();
}

//String currentLine;
String runCommand(String fullCommand);
void loop() {
  ArduinoOTA.handle();

  yield();
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char incomingPacket[CMDBUFFERSIZE];
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
    
    const char* reply = runCommand(incomingPacket);

    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(reply);
    Udp.write('\n');
    Udp.endPacket();
  }
}

const char* runCommand(char* fullCommand) { // returns status of command
  Serial.println(fullCommand);

  int index = 0;
  char* split[50];
  //char* cFullCommand = strdup(fullCommand.c_str()); // ... this needs to be free'd

  char* command = strtok(fullCommand, " ");
  while (command != 0) {
    split[index] = command;
    index++;
    command = strtok(0, " ");
  }
  Serial.print("Index : "); Serial.println(index);
  yield();
  
  if (index == 3 && strcmp(split[0], "mov") == 0) { // mov pwm time
    int pwm = atoi(split[1]);
    int del = atoi(split[2]);
    
    analogWrite(leftWheel, pwm);
    analogWrite(rightWheel, pwm);
    delay(del);
    analogWrite(leftWheel, 0);
    analogWrite(rightWheel, 0);
    return "OK";
  }

  if (index == 3 && strcmp(split[0], "set") == 0) { // set left/right pwm
    if (atoi(split[1])) {
      analogWrite(leftWheel, atoi(split[2]));
    } else {   
      analogWrite(rightWheel, atoi(split[2]));
    }

    goto OK;
  }

  // sets both at the same time
  if (index == 3 && strcmp(split[0], "both") == 0) { // both leftPWM rightPWM
    /*analogWrite(leftWheel, atoi(split[1]));
    analogWrite(rightWheel, atoi(split[2]));*/
    motors[0].curPWM = atoi(split[1]);
    motors[1].curPWM = atoi(split[2]);
    goto OK;
  }
  
  //return "OK";
  //free(cFullCommand);
  return "Invalid Command";
OK:
  //free(cFullCommand);
  return "OK";
}

