// ESP-Clock v.07 - March 2018
// A simple clock synced with NTP
// This version is for an ESP-01 and TM1637 4-digit, 7-segment display with colon.
// 

#include "TM1637.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
//https://github.com/tzapu/WiFiManager
#include <WiFiManager.h>

// Setup the display depending on which ESP8266 is being used. (clk, di0)
// ESP826612
// CLK - D7, DI0 - D5
// Uncomment below for ESP826612
// TM1637 tm1637(13, 14);
// ESP-01
// CLK - GPI2, DI0 - GPI0
// Uncomment below for ESP-01
TM1637 tm1637(2,0);

// TIMEZONE 19 = EST, -5
// Create a list of the other zones
#define TIMEZONE 20

// local port to listen for UDP packets
unsigned int  localPort = 2390;
// how often make correct (min)
unsigned int ntp_corr = 1;
// Correct counter
unsigned int ntp_count = 0;
// TIME
unsigned long ntp_time = 0;
 // blinking colon points
bool points = true;

// Configure the NTP server to use
IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

WiFiUDP udp;

void setup()
{
  // CLEAR DISP
  tm1637.init();
  // Set the brightness DARKEST = 0, BRIGHTEST = 7
  tm1637.set(5);
  for ( int i = 0; i < 4; i++) {
    tm1637.display(i, i);
    delay(300);
  }

  Serial.begin(115200);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    wifiManager.resetSettings();

    //set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("wrightMac_clock");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();


    //if you get here you have connected to the WiFi
    //Serial.println("connected.. . what time is it?");

  // START UDP connect NTP
  udp.begin(localPort);

  while (ntp_time == 0) GetNTP();
}

void loop() {
  tm1637.init();
  //delay(1000);
  tm1637.display(0, ( ( ntp_time / 3600 ) % 24 ) / 10);
  //delay(1000);
  tm1637.display(1, ( ( ntp_time / 3600 ) % 24 ) % 10);
  //delay(1000);
  tm1637.display(2, ( ( ntp_time / 60 ) % 60 ) / 10);
  //delay(1000);
  tm1637.display(3, ( ( ntp_time / 60 ) % 60 ) % 10);
  // For debuggin. This can be commented out or removed
  /*Serial.print("NTP Time raw:");
  Serial.print (ntp_time);
  Serial.print("NTP Time / 3600:");
  Serial.print(ntp_time /3600);
  */

  ntp_time++;
  ntp_count++;
  tm1637.point(points);
  points = !points;
  delay(1000);
  //if ( ntp_count == ( ntp_corr * 60 ) ) GetNTP();
  if (ntp_count == ( ntp_corr *1 ) ) GetNTP();
}

// NTP server request
bool GetNTP(void) {
  WiFi.hostByName(ntpServerName, timeServerIP);

  // Keep us in the loop of what is going on
  //Serial.println("Sending NTP packet...");

  // clear to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  //
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  //
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // NTP to 123 port
  udp.beginPacket(timeServerIP, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

  delay(3000);
  int cb = udp.parsePacket();
  if (!cb) {
    //Serial.println("No packet...");

    //BAD NTP
    for ( int i = 0; i < 4; i++) {
      tm1637.display(i, 8);
      delay(100);
    }
    delay (100);
    tm1637.init(); // CLEAR DISP
    return false;
  }
  else { 
   // Read the package into the buffer
   udp.read(packetBuffer, NTP_PACKET_SIZE);
   // 4 bytes starting from the 40th will hold the time stamping time - the number of seconds
   // from 01/01/1900
   unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
   unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
   // Convert two words to a long variable
   unsigned long secsSince1900 = highWord << 16 | lowWord;
   // Convert to UNIX-timestamp (the number of seconds from 01/01/1970
   const unsigned long seventyYears = 2208988800UL;
   unsigned long epoch = secsSince1900 - seventyYears;
   // Make a correction for the local time zone
   ntp_time = epoch + TIMEZONE * 3600;
   Serial.print("Unix time = ");
   Serial.println(ntp_time);
   ntp_count = 0;
  }
  return true;
}

