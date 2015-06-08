//  ultracar_v1.2.ino
//
//  Created by guang zhou & Seungcheul Kim on 6/3/15.
//  Copyright (c) 2015 guang zhou & Seungcheul Kim. All rights reserved.
//
#include <WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>

// Sonic wave control
#define echoPin 7 // Echo Pin
#define trigPin 18 // Trigger Pin

// motor pin-map 
#define A_1L 5
#define A_1H 2
#define A_2H 6
#define A_2L 30 //7  (wiring pin 30 in cc3200 to pin 7 in motor board)
#define B_1L 28 //13 (wiring pin 28 in cc3200 to pin 13 in motor board) 
#define B_1H 32 //12 (wiring pin 32 in cc3200 to pin 12 in motor board)
#define B_2H 14
#define B_2L 15
#define A_sleep  8
#define B_sleep  17

//config these parameters
int thisdistance = 1000;
int minidistance = 70;
int turntime = 200;
int servodelay = 300;
int servostate = 2; //1 left, 2 mid, 3 right, 4 mid.
unsigned long timer0 = 2000;
unsigned long timer1 = 2000;
unsigned long timer2 = 0;
boolean flag = 0;
boolean flagmsg = 0;
int rundistance = 2000;

// the network name you want to created
char ssid[] = "gzhou358";
// your network password
char password[] = "12345678";
WiFiServer server(80);
boolean alreadyConnected = false; // whether or not the client was connected previously
char charBuf[20];
char phoneBuf[20];

int execute = 0;
int command = 0;
String inData;
String outData;

Servo myservo; // Create servo object to control a servo
               // a maximum of eight servo objects can be created

int pos = 0;  // variable to store the servo position
long duration, distance; // Duration used to calculate distance

void setup(){
  // initialize serial communication at 9600 bits per second:
  initPins();
  myservo.attach(29);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
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
        client.print(charBuf);
        if(!strncmp(charBuf,"ledh",4)){
          execute =1; 
          command =1;
        }
        if(!strncmp(charBuf,"ledl",4)){
          execute =1; 
          command =2;
        }
        if(!strncmp(charBuf,"start",5)){
          execute =1; 
          command =3;
          fwd();
          timer1 = millis();
        }
        if(!strncmp(charBuf,"forward",6)){
          fwd();
        }
        if(!strncmp(charBuf,"turnL",5)){
          leftT();
        }
        if(!strncmp(charBuf,"turnR",5)){
          rightT();
        }
        if(!strncmp(charBuf,"rev",3)){
          rev();
        }
        if(!strncmp(charBuf,"servoL",6)){
          servoL();
        }
        if(!strncmp(charBuf,"servoF",6)){
          servoF();
        }
        if(!strncmp(charBuf,"servoR",6)){
          servoR();
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
      digitalWrite(YELLOW_LED, HIGH);
      Serial.println("LED H");
    }
    if (command == 2) {
      execute = 0;
      digitalWrite(YELLOW_LED, LOW);
      Serial.println("LED L");
    }
    if (command == 3) {
      //get current time for this round.
      timer2 = millis();
      if ((timer2-timer1)>=servodelay){
        thisdistance = scan();
        Serial.println(thisdistance);
        switch (servostate) {
          case 1: // last L, next F
            while (thisdistance < minidistance){
              if (!flag){
                client.print("hasobstacle");
                flag = 1;  flagmsg = 1;
              }
              timer0 = timer2;
              rightT();
              sleep(turntime/2);
              thisdistance = scan();
              Serial.println(thisdistance);
            }
            servoF(); servostate =2;
            break;   
          case 2: // last F, next R
            while (thisdistance < minidistance){
              if (!flag){
                client.print("hasobstacle");
                flag = 1;  flagmsg = 1;
              }
              timer0 = timer2;
              rightT();
              sleep(turntime);
              thisdistance = scan();
              Serial.println(thisdistance);
            }
            servoR(); servostate =3;
            break;   
          case 3: // last R, next F
            while (thisdistance < minidistance){
              if (!flag){
                client.print("hasobstacle");
                flag = 1;  flagmsg = 1;
              }
              timer0 = timer2;
              leftT();
              sleep(turntime/2);
              thisdistance = scan();
              Serial.println(thisdistance);
            }
            servoF(); servostate =4;
            break; 
          case 4: // last F, next L
            while (thisdistance < minidistance){
              if (!flag){
                client.print("hasobstacle");
                flag = 1;  flagmsg = 1;
              }
              timer0 = timer2;
              leftT();
              sleep(turntime);
              thisdistance = scan();
              Serial.println(thisdistance);
            }
            servoL(); servostate =1;
            break; 
          default:
            break;
        }
        if(flag){
          flag = 0; fwd();
        }
        if(flagmsg&&((timer2-timer0)>=rundistance)){
          timer0 = millis();
          client.print("noobstacle"); 
          flagmsg = 0;
        }
        timer1 = millis();
      }
    }
  }  
}

void servoR(){
  myservo.write(40);
}
void servoL(){
  myservo.write(150);
}
void servoF(){
  myservo.write(95);
}

int scan(){
  /* The following trigPin/echoPin cycle is used to determine the
  distance of the nearest object by bouncing soundwaves off of it. */
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
 //Calculate the distance (in cm) based on the speed of sound.
  distance = duration/58.2;
  return distance;
}

void fwd() {
  leftF();
  rightF();
}

void rev() {
 leftR();
 rightR();
}

void rightT() {
  leftF();
  rightR();
}

void leftT() {
  leftR();
  rightF();
}

void brake() {
  digitalWrite (A_1L, LOW);
  digitalWrite (A_1H, LOW);
  digitalWrite (A_2L, LOW);
  digitalWrite (A_2H, LOW);
  digitalWrite (B_1L, LOW);
  digitalWrite (B_1H, LOW);
  digitalWrite (B_2L, LOW);
  digitalWrite (B_2H, LOW);
}

void leftF() {
  digitalWrite (A_1L, LOW);
  digitalWrite (A_1H, HIGH);
  digitalWrite (A_2L, HIGH);
  digitalWrite (A_2H, LOW);
}

void rightF() {
  digitalWrite (B_1L, LOW);
  digitalWrite (B_1H, HIGH);
  digitalWrite (B_2L, HIGH);
  digitalWrite (B_2H, LOW);
}

void leftR() {
  digitalWrite (A_1L, HIGH);
  digitalWrite (A_1H, LOW);
  digitalWrite (A_2L, LOW);
  digitalWrite (A_2H, HIGH);
}

void rightR() {
  digitalWrite (B_1L, HIGH);
  digitalWrite (B_1H, LOW);
  digitalWrite (B_2L, LOW);
  digitalWrite (B_2H, HIGH);
}

void initPins() {
  pinMode(A_sleep, OUTPUT);
  pinMode(B_sleep, OUTPUT);
  digitalWrite(A_sleep, LOW);
  digitalWrite(B_sleep, LOW);
  pinMode(A_1L, OUTPUT);
  pinMode(A_1H, OUTPUT);
  pinMode(A_2H, OUTPUT);
  pinMode(A_2L, OUTPUT);
  pinMode(B_1L, OUTPUT);
  pinMode(B_1H, OUTPUT);
  pinMode(B_2H, OUTPUT);
  pinMode(B_2L, OUTPUT);
  digitalWrite(A_1L, LOW);
  digitalWrite(A_1H, LOW);
  digitalWrite(A_2H, LOW);
  digitalWrite(A_2L, LOW);
  digitalWrite(B_1L, LOW);
  digitalWrite(B_1H, LOW);
  digitalWrite(B_2H, LOW);
  digitalWrite(B_2L, LOW);
  digitalWrite(A_sleep, HIGH);
  digitalWrite(B_sleep, HIGH);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(ssid);
  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("Server IP Address: ");
  Serial.println(ip);
}
