#include <SPI.h>
#include "SparkFunLSM6DS3.h"
#include "Wire.h"
#include "WiFiEsp.h"
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(10, 11); // RX, TX
#endif

#define BUZZER 5

char ssid[] = "ResNet Mobile Access";         // Network SSID (name)
char pass[] = "";                             // Network password
int status = WL_IDLE_STATUS;                  // the Wifi radio's status
char server[] = "52.91.164.206";              // AWS server public IP address
char get_request[200];                        // HTTP request to server with data

WiFiEspClient client;                         // Initialize Ethernet client object

LSM6DS3 accelerometer;                        // Default constructor is I2C, addr 0x6B

int stepCounter;         // Current number of steps taken
float minY;              // Use for movement calibration 
float maxY;              // Use for movement calibration
int timeStart;           // Use for calibration timing
int readDelay;           // How often to read accelerometer
bool dirUp;              // Did accelerometer go upwards?
bool dirDown;            // Did accelerometer go downwards?    
int exerciseStartTime;   // Keep track of how long user been exercising   
int exerciseDuration;    // How long user wants to exercise for
bool doneExercising;     // Check if done exercising yet
bool calibrating;        // Check if still calibrating
bool startedExercise;    // Check if started exercise yet
bool buzzed;             // Check if already notified that time is up

void setup()
{
  stepCounter = 0;           // Number of steps taken
  minY = -100;               // Move up = -Y  
  maxY = 100;                // Move down = +Y
  readDelay = 500;           // Read accelerometer every 20 ms
  dirUp = false;             // Must go up then down to count as step
  dirDown = true;            // Start down
  doneExercising = false;    
  exerciseDuration = 10000;  // Exercise duration specified by user
  calibrating = true;        // Start with calibration
  startedExercise = false;
  buzzed = false;        
  pinMode(BUZZER, OUTPUT);   // Setup buzzer for when exercise time is up
  Serial.begin(115200);      // Initialize serial for debugging
  Serial1.begin(115200);     // initialize serial for ESP module

  accelerometer.begin();
  timeStart = millis();
}

void loop()
{ 
  // Check if done exercising yet
  if(startedExercise && isDoneExercising())  // done exercising
  {
    Serial.println("DONE EXERCISING");
    doneExercising = true;
    if(!buzzed) {                // Only buzz once
      buzz();                    // Buzz to alert user that time is up
      WiFi.init(&Serial1);       // Initialize ESP module

      // Attempt to connect to WiFi network
      while(status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);   // Connect to WPA/WPA2 network
      }
      Serial.println("Successfully connected to the network");
      printWifiStatus();
      connectToServer();
      sendData();
    }
  }
  
  if(!doneExercising)  // Not done exercising yet so keep going
  {
    // Calibrate for 10 seconds to find best values for detecting movement
    if((millis() - timeStart) < 10000)  // Still calibrating
      calibrate(); 
    else                                // Done calibrating, do step counting
    {
      calibrating = false;
      if(!startedExercise) {
        startedExercise = true;
        exerciseStartTime = millis();
      }

      updateStepCount();
      Serial.print("Step Count: ");
      Serial.println(stepCounter);
      delay(200);
    }
  }
}

void printWifiStatus()
{
  // print SSID of network attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println("dBm");
}

void connectToServer()
{
  Serial.println();
  if(!client.connected()) {
    Serial.println("Starting connection to server...");
    client.connect(server, 5000);
  }
  Serial.println("Connected to server");
}

bool isDoneExercising()
{
  int curTime = millis();
  return (curTime - exerciseStartTime) >= exerciseDuration;
}

void buzz()
{
  digitalWrite(BUZZER,  HIGH);  // Make buzzer go off
  delay(1000);
  digitalWrite(BUZZER, LOW);
  buzzed = true;                // Only buzz once after time is up
}

void sendData()
{
  // Make HTTP request to send data to server
  sprintf(get_request, "GET /add?dur=%d&cnt=%d HTTP/1.1\r\nHost: 52.91.164.206\r\nConnection: close\r\n\r\n", exerciseDuration, stepCounter);  // send duration and step count
  client.print(get_request);
  
  while(client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  delay(500);
}

void calibrate()
{
  float curY = accelerometer.readFloatAccelY();
  Serial.print("cur Y: ");
  Serial.println(curY);
  if(curY > maxY)
    maxY = curY;
  if(curY <= minY)
    minY = curY;
}

void updateStepCount()
{
  float curY = accelerometer.readFloatAccelY();
  if(curY < 0)
    dirUp = true;
  else
    dirDown = true;

  if(dirUp && dirDown) {
    stepCounter++;     // Must move up and down to count as step
    dirUp = false;     // Reset for next step
    dirDown = false;   // Reset for next step
  }
}
