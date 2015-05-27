//  ultracar_v1.0.ino
//
//  Created by guang zhou & Seungcheul Kim on 5/20/15.
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

//config the parameters
int leftdistance = 1000;
int rightdistance = 1000;
int frontdistance = 1000;
int sensordistance = 80;
int turntime = 200;
int servodelay = 300;

// the network name you want to created
char ssid[] = "asd1234";
// your network password
char password[] = "12345678";
WiFiServer server(80);
boolean alreadyConnected = false; // whether or not the client was connected previously
char charBuf[20];
int execute = 0;
int command = 0;
String inData;

Servo myservo; // Create servo object to control a servo
               // a maximum of eight servo objects can be created

int pos = 0;  // variable to store the servo position
long duration, distance; // Duration used to calculate distance

void setup()
{
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
        }
        //
        if(!strncmp(charBuf,"forward",6)){
          execute =1;
          fwd();
        }
        if(!strncmp(charBuf,"turnL",5)){
          execute =1;
          leftT();
        }
        if(!strncmp(charBuf,"turnR",5)){
          execute =1;
          rightT();
        }
        if(!strncmp(charBuf,"rev",3)){
          execute =1;
          rev();
        }
        if(!strncmp(charBuf,"servoL",6)){
          execute =1;
          servoL();
        }
        if(!strncmp(charBuf,"servoR",6)){
          execute =1;
          servoR();
        }
        if(!strncmp(charBuf,"stop",4)){
          execute = 0; 
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
        fwd();
        leftdistance = scanL();
        client.print(leftdistance);
        Serial.println(leftdistance);
        if (leftdistance < sensordistance){
          brake();
          while (leftdistance < sensordistance){
            rightT();
            sleep(turntime/2);
            brake();
            leftdistance = scan();
            client.print(leftdistance);
            Serial.println(leftdistance);
          }
        }
        
        fwd();
        frontdistance = scanF();
        client.print(frontdistance);
        Serial.println(frontdistance);
        if (frontdistance < sensordistance){
          brake();
          while (frontdistance < sensordistance){
            rightT();
            sleep(turntime);
            brake();
            frontdistance = scan();
            client.print(frontdistance);
            Serial.println(frontdistance);
          }
        }
        
        fwd();
        rightdistance = scanR();
        Serial.println(rightdistance);
        client.print(rightdistance);
        if (rightdistance < sensordistance){
          brake();
          while (rightdistance < sensordistance){
            leftT();
            sleep(turntime/2);
            brake();
            rightdistance = scan();
            client.print(rightdistance);
            Serial.println(rightdistance);
          }
        }
        
        fwd();
        frontdistance = scanF();
        client.print(frontdistance);
        Serial.println(frontdistance);
        if (frontdistance < sensordistance){
          brake();
          while (frontdistance < sensordistance){
            leftT();
            sleep(turntime);
            brake();
            frontdistance = scan();
            client.print(frontdistance);
            Serial.println(frontdistance);
          }
        }
    }
  }  
}

void servoR()
{
  myservo.write(40);
  delay(servodelay);
}
void servoL()
{
  myservo.write(150);
  delay(servodelay);
}
void servoF()
{
  myservo.write(95);
  delay(servodelay);
}

int scan()
{
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

int scanR(){
  servoR();  
  return scan();
}

int scanL(){
  servoL();
  return scan();
}

int scanF(){
  servoF();  
  return scan();
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
