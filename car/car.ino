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
    //char incomingPacket[sizeof(struct control)/sizeof(char)];
    ControlPacket controlPacket;
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(reinterpret_cast<char *>(&controlPacket), sizeof(ControlPacket));
    if(len != sizeof(ControlPacket)) return;

    //struct control controlPacket = reinterpret_cast<struct control>(incomingPacket);
    for(int i = 0; i <= 1; i++) {
      motors[i].curPWM = controlPacket.pwm[i];
    }

    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(reinterpret_cast<char *>(&stat), sizeof(struct StatusPacket));
    Udp.endPacket();
  }
}


