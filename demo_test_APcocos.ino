/***************************************************
  This is an example for the Adafruit CC3000 Wifi Breakout & Shield

  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

/*
  This example does a test of the TCP client capability:
   Initialization
   Optional: SSID scan
   AP connection
   DHCP printout
   DNS lookup
   Optional: Ping
   Connect to website and print out webpage contents
   Disconnect
  SmartConfig is still beta and kind of works but is not fully vetted!
  It might not work on all networks!
*/
#include <DHT.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                         SPI_CLOCK_DIV4); // you can change this clock speed

#define WLAN_SSID       "cocos"           // cannot be longer than 32 characters!
#define WLAN_PASS       "123456789o"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
// received before closing the connection.  If you know the server
// you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "192.168.43.58"  // ex. "www.adafruit.com"
#define WEBPAGEDATA      "/serverTest/connectSQL.php"     // ex. "/testwifi/index.html"
#define WEBPAGECMD      "/serverTest/getData.php"     // ex. "/testwifi/index.html"
/************************/
/*Sensor DHT11 XD28 set */
/************************/

#define RALAY  9
#define DHTPIN 2
#define DHTTYPE 11
#define SENSORPIN A0
//#define SENSORDELAY 1000
DHT dht (DHTPIN, DHTTYPE);
int h = 0;
int t = 0;
int soilValue = 0;
int count = 0;
Adafruit_CC3000_Client client;
#define BUFFER_SIZE 64
char buf[BUFFER_SIZE + 1];
String sensorValue = "";
String cmd = "";
uint32_t ip;
//const unsigned long connectTimeout  = 15L * 1000L; // Max time to wait for server connection
const unsigned long connectTimeout  = 3000; // Max time to wait for server connection
const unsigned long responseTimeout = 3000; // Max time to wait for data from server
unsigned long startTime;
unsigned long lastRead;
/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

void setup(void)
{
  Serial.begin(9600);
  dht.begin();
  pinMode(RALAY, OUTPUT);

  Serial.println(F("Hello, CC3000!\n"));
    Serial.print(F("Free RAM: ")); Serial.println(getFreeRam(), DEC);

  /* Initialise the module */
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while (1);
  }

  Serial.println(F("Initializing net"));
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while (1);
  }

  Serial.println(F("net Connected!  "));

  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  /********************/
  /*Connection Servers*/
  /********************/

  ip = 0xC0A82B3A;      /*"www.ab126.com/system/2859.html"*/
  Serial.println(F("Connecting server"));
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  cc3000.printIPdotsRev(ip);
  Serial.print(F("\nserver Connected! "));
  Serial.print(F("\r\n"));
  Serial.println(F("*************************************"));

}
// Optional: Do a ping test on the website
/*
  Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print("...");
  replies = cc3000.ping(ip, 5);
  Serial.print(replies); Serial.println(F(" replies"));
*/

/* Try connecting to the website.
   Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
*/


void loop(void)
{ 
  delay(2000);
  sensor();
  delay(2000);
  uploadData();
  delay(2000);
  count++;
  Serial.print(F("count: "));Serial.println(count);
  if (soilValue < 30) {
    for (int i = 0 ; i < 6 ; i++) {
      delay(5000);
      commandData();
      delay(2000);
      sensor();     
      if(soilValue>30){
        break;
      }
    }
  }
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
void sensor() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  soilValue = analogRead(SENSORPIN); //土壤濕度感測模組
  soilValue = map(soilValue, 250, 1023, 100, 0); //將數值轉換為0%~100%表示

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print(F("Humidity: "));  /*環境濕度*/
  Serial.print(h);
  Serial.print(F(" %\t"));
  Serial.print(F("Temperature: "));  /*環境溫度*/
  Serial.print(t);
  Serial.print(F(" *C\t"));
  Serial.print(F("Soil: "));  /*土壤濕度*/
  Serial.print(soilValue);
  Serial.println(F(" % "));

}
/************************************************************/
/*uploadData*/
void uploadData() {
  Serial.print(F("Free RAM: ")); Serial.println(getFreeRam(), DEC);
  sensorValue = String("/serverTest/connectSQL.php?XD28=") + soilValue +
                String("&DHT11_T=") + t + String("&DHT11_H=") + h;
  sensorValue.trim();
  Serial.print("sensorValue string length:"); Serial.println(sensorValue.length(), DEC);
  if (sensorValue.length() > BUFFER_SIZE) {
    Serial.println("buf overflow");
  }
  sensorValue.toCharArray(buf, BUFFER_SIZE);
  buf[sensorValue.length()] = 0;
  Serial.print(F("buf Free RAM: ")); Serial.println(getFreeRam(), DEC);
  Serial.print(WEBSITE);
  Serial.print(buf);
  Serial.print(F("\n"));
  startTime = millis();
  do {
    client = cc3000.connectTCP(ip, 80); /*試著連線*/
    Serial.print(F("connect status:"));
    Serial.println(client.connected());
  } while ((!client.connected()) &&
           ((millis() - startTime) < connectTimeout));
  //  Serial.print("connect status:");
  //  Serial.println(client.connected());
  if (client.connected()) {
    Serial.print(F("\nuploadData connected OK"));
    Serial.print(F("\n"));

    client.fastrprint(F("GET "));
    client.fastrprint(buf);
    client.fastrprint(F(" HTTP/1.1\r\n"));
    client.fastrprint(F("Host: ")); client.fastrprint(WEBSITE); client.fastrprint(F("\r\n"));
    client.fastrprint(F("\r\n"));
    client.println();

    startTime = millis();
    while ((!client.available()) &&
           ((millis() - startTime) < responseTimeout));


    Serial.print(F("upload data..."));
    Serial.print(F("\n"));
    Serial.println(F("-------------------------------------"));
    client.close();
  } else {
    Serial.println(F("uploadData Connection failed"));
    return;
  }
}
/********************/
/*Connection Servers*/
/********************/
void commandData() {
  do {
    client = cc3000.connectTCP(ip, 80);
    Serial.print(F("connect status:"));
    Serial.println(client.connected());/*試著連線*/
  } while ((!client.connected()) &&
           ((millis() - startTime) < connectTimeout));
  if (client.connected()) {
    Serial.print(F("\ncommandData connected OK"));
    Serial.print(F("\n"));

    client.fastrprint(F("GET "));
    client.fastrprint(WEBPAGECMD);
    client.fastrprint(F(" HTTP/1.1\r\n"));
    client.fastrprint(F("Host: ")); client.fastrprint(WEBSITE); client.fastrprint(F("\r\n"));
    client.fastrprint(F("\r\n"));
    client.println();

    lastRead = millis();
    while ((client.connected()) &&
           ((millis() - lastRead) < responseTimeout)) {
      while (client.available()) {
        char c = client.read();
        //        Serial.print(c);
        cmd = cmd + c;
        lastRead = millis();
      }
    }

    Serial.print(F("\n"));
    Serial.print(F("command data..."));
    Serial.print(F("\n"));
    Serial.println(F("-------------------------------------"));
    Serial.print(F("command Free RAM: ")); Serial.println(getFreeRam(), DEC);
    client.close();
    if (cmd.endsWith("@")) {
      Serial.print(F("RALAY: "));
      Serial.print(F("OPEN"));
      Serial.print(F("\r\n"));
      Serial.println(F("-------------------------------------"));
      digitalWrite(RALAY, HIGH);
      delay(3000);
      digitalWrite(RALAY, LOW);      
    } 
    return;
  } else {
    Serial.println(F("commnadData Connection failed"));
    return;
  }
}
