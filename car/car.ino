#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const int leftWheel = 4;
const int rightWheel = 5;
const int statusLED = 16;

#include "packets.h"
#include "utils.hpp"
#include "control.hpp"

const int CMD_BUFFER_SIZE = (sizeof(struct ControlPacket)/sizeof(char)) + 1;

WiFiUDP Udp;

void setup() {
  pinMode(statusLED, OUTPUT);
  digitalWrite(statusLED, LOW);
  
  Serial.begin(115200);
  Serial.println(sizeof(ControlPacket));
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
  analogWriteFreq(2);
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
    char incomingPacket[CMD_BUFFER_SIZE];
    ControlPacket controlPacket;
    
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, CMD_BUFFER_SIZE);
    if(len != sizeof(struct ControlPacket)) return;
    memcpy(&controlPacket, incomingPacket, sizeof(struct ControlPacket)); // get rid of the null terminator character

    //struct control controlPacket = reinterpret_cast<struct control>(incomingPacket);
    for(int i = 0; i <= 1; i++) {
      int currentSpeed = controlPacket.pwm[i]; // implicit type casting
      Serial.print("Recv packet motor ");
      Serial.print(i);
      Serial.print(" speed : ");
      Serial.println(controlPacket.pwm[i]);
      setMotorPWM(motors[i], currentSpeed);
    }

    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(reinterpret_cast<char *>(&stat), sizeof(struct StatusPacket));
    Udp.endPacket();
  }
}


