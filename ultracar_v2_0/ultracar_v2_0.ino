//  ultracar_v5_0.ino
//
//  Created by guang zhou on 7/22/15.
//  Copyright (c) 2015 guang zhou. All rights reserved.
//
#include <WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>
// Ultra sound wave control
#define echoPin1 3 // Echo Pin
#define trigPin1 4 // Trigger Pin
#define echoPin2 5 // Echo Pin
#define trigPin2 6 // Trigger Pin
#define echoPin3 7 // Echo Pin
#define trigPin3 8 // Trigger Pin
int scanState = 1; //1 left, 2 mid, 3 right
int scanDelay = 25; 
// motor control
Servo motorL;
Servo motorR;
boolean isInitMotors = false;
int motorNeutral = 100;
int motorLout = motorNeutral;
int motorRout = motorNeutral;
// the wifi network 
char ssid[] = "gzhou358";
char password[] = "12345678";
WiFiServer server(80);
boolean alreadyConnected = false; // whether or not the client was connected previously
String ouData;
String inData;
char phoneBuf[20];
char charBuf[20];
char numBuf[3];
//init system parameter
unsigned long timer1 = 2000;
unsigned long timer2 = 0;
int execute = 0;
int command = 0;
int i;

void setup(){
  // initialize serial communication at 9600 bits per second:
  motorL.attach(30);
  motorR.attach(31);
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT);
  pinMode(echoPin3, INPUT);
  Serial.begin(115200);      // initialize serial communication
  pinMode(YELLOW_LED, OUTPUT);      // set the LED pin mode
  Serial.print("Starting create network...");
  WiFi.beginNetwork(ssid, password);
  Serial.println("network created done.");
  printWifiStatus();
  Serial.println("Starting webserver on port 80");
  server.begin();                           // start the web server on port 80
  Serial.println("Webserver started!");
}

void loop(){
  if (!isInitMotors) {
    motorL.write(motorNeutral);
    motorR.write(motorNeutral);
    isInitMotors = true;
    Serial.println("motor and servo initialized");
  }
  WiFiClient client = server.available();   // listen for incoming clients
  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clead out the input buffer:
      client.flush();
      Serial.println("We have a new client");
      client.print("Hello, client!");
      alreadyConnected = true;
    }    
    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();      
      // Process message when new line or carriage return character is recieved
      inData += thisChar;
      if(thisChar == '\n' || thisChar == '\r'){
        inData.toCharArray(charBuf, 20);
        Serial.print("received command ");
        Serial.print(charBuf);
        //1 left, 2 mid, 3 right
        if(!strncmp(charBuf,"scanL",5)){
          execute =1; 
          command =1;
        }
        if(!strncmp(charBuf,"scanF",5)){
          execute =1; 
          command =2;
        }
        if(!strncmp(charBuf,"scanR",5)){
          execute =1; 
          command =3;
        }
        if(!strncmp(charBuf,"ledh",4)){
          execute =1; 
          command =4;
        }
        if(!strncmp(charBuf,"ledl",4)){
          execute =1; 
          command =5;
        }
        if(!strncmp(charBuf,"start",5)){
          execute =1; 
          command =6;
          timer1 = millis();
        }
        if(!strncmp(charBuf,"motor",5)){
          for(i=0;i<5;i++){
            numBuf[i]=charBuf[i+6];
          }
          motorLout = atoi(numBuf)-1000;
          Serial.print("Lout ");
          Serial.println(motorLout);
          motorL.write(motorLout);
          for(i=0;i<5;i++){
            numBuf[i]=charBuf[i+11];
          }
          motorRout = atoi(numBuf)-1000;
          Serial.print("Rout ");
          Serial.println(motorRout);
          motorR.write(motorRout);
        }
        if(!strncmp(charBuf,"stop",4)){
          execute = 0;
          command = 0; 
          brake();
        }
        inData = ""; // Clear recieved buffer
      }
    }
  }
  if (execute == 1) {
    if (command == 1) {
      execute = 0;
      ouData = "oL ";
      ouData = ouData + sampling(1);
      ouData.toCharArray(phoneBuf, 20);
      client.print(phoneBuf);
    }
    if (command == 2) {
      execute = 0;
      ouData = "oF ";
      ouData = ouData + sampling(2);
      ouData.toCharArray(phoneBuf, 20);
      client.print(phoneBuf);
    }
    if (command == 3) {
      execute = 0;
      ouData = "oR ";
      ouData = ouData + sampling(3);
      ouData.toCharArray(phoneBuf, 20);
      client.print(phoneBuf);
    }
    if (command == 4) {
      execute = 0;
      digitalWrite(YELLOW_LED, HIGH);
      Serial.println("LED H");
    }
    if (command == 5) {
      execute = 0;
      digitalWrite(YELLOW_LED, LOW);
      Serial.println("LED L");
    } 
    if (command == 6) {
      timer2 = millis();
      ouData = "";
      if ((timer2-timer1)>=scanDelay){
        switch (scanState) {
          //1 left, 2 mid, 3 right
          case 1: // This L, next F
            ouData = "oL ";
            ouData = ouData + sampling(1);
            ouData.toCharArray(phoneBuf, 20);
            client.print(phoneBuf);
            scanState =2;
            break;   
          case 2: // This F, next R
            ouData = "oF ";
            ouData = ouData + sampling(2);
            ouData.toCharArray(phoneBuf, 20);
            client.print(phoneBuf);
            scanState =3;
            break; 
          case 3: // This R, next L
            ouData = "oR ";
            ouData = ouData + sampling(3);
            ouData.toCharArray(phoneBuf, 20);
            client.print(phoneBuf);
            scanState =1;
            break;   
          default:
            break;
        }
        timer1 = millis();
      }
    }
  }  
}
void brake() {
  motorL.write(motorNeutral);
  motorR.write(motorNeutral);
}
void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(ssid);
  IPAddress ip = WiFi.localIP();
  Serial.print("Server IP Address: ");
  Serial.println(ip);
}
int sampling(int id){
  //scan 3 times, then take the avg.
  int avgdistance = 0;
  for(int i=0;i<3;i++){
    avgdistance = avgdistance + scan(id);
  }
  avgdistance = avgdistance / 3;
  return avgdistance;
}
int scan(int id){
  //time cost is (1-6) milis. depends on the distance
  //maximum detecting range is 135cm. otherwise return 200.
  //long a = millis();
  long duration, distance;
  switch (id){
    case 1:
      digitalWrite(trigPin1, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin1, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin1, LOW);
      duration = pulseIn(echoPin1, HIGH, 3000);
      break;
    case 2:
      digitalWrite(trigPin2, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin2, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin2, LOW);
      duration = pulseIn(echoPin2, HIGH, 3000);
      break;
    case 3:
      digitalWrite(trigPin3, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin3, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin3, LOW);
      duration = pulseIn(echoPin3, HIGH, 3000);
      break;
    default:
      break;
  }
  distance = duration/58.2;
  //Serial.print("time cost is ");
  //Serial.println(millis()-a);
  if (distance <= 1)
    return 200;
  else 
    return distance;
}
