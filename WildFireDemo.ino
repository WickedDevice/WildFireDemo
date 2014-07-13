
/***************************************************
  This sketch is a mashup of code from Adafruit_CC3000_Library
  examples, CC3000_MDNS, and RESTduino code cc3kserver
   
  Adafruit CC3000 Multicast DNS Web Server
    
  This is a simple example of an echo server that responds
  to multicast DNS queries on the 'wildfire.local' address.

  See the CC3000 tutorial on Adafruit's learning system
  for more information on setting up and using the
  CC3000:
    http://learn.adafruit.com/adafruit-cc3000-wifi  
    
  Requirements:
  
  You must also have MDNS/Bonjour support installed on machines
  that will try to access the server:
  - For Mac OSX support is available natively in the operating
    system.
  - For Linux, install Avahi: http://avahi.org/
  - For Windows, install Bonjour: http://www.apple.com/support/bonjour/
  
  Usage:
    
  Update the SSID and, if necessary, the CC3000 hardware pin 
  information below, then run the sketch and check the 
  output of the serial port.  Once listening for connections, 
  connect to the server from your computer  using a telnet 
  client on port 7.  
           
  For example on Linux or Mac OSX, if your CC3000 has an
  IP address 192.168.1.100 you would execute in a command
  window:
  
    telnet wildfire.local 7

  License:
 
  This example is derivative work based on work the wwwServer example 
  copyright (c) 2013 Tony DiCola (tony@tonydicola.com)
  and is released under an open source MIT license.  See details at:
    http://opensource.org/licenses/MIT
  
  This code was adapted from Adafruit CC3000 library example 
  code which has the following license:
  
  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor   Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution      
 ****************************************************/
#include <WildFire_CC3000.h>
#include <SPI.h>
#include <WildFire_CC3000_MDNS.h>
#include <WildFire.h>
WildFire wf;

//#define DEBUG 1

WildFire_CC3000 cc3000;

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           80    // What TCP port to listen on for connections.

WildFire_CC3000_Server wwwServer(LISTEN_PORT);

MDNSResponder mdns;

boolean attemptSmartConfigReconnect(void);
boolean attemptSmartConfigCreate(void);

void setup(void)
{  
  wf.begin();
  
  Serial.begin(115200);
  Serial.println(F("Welcome to WildFire!\n")); 
    
  if(!attemptSmartConfigReconnect()){
    while(!attemptSmartConfigCreate()){
      Serial.println(F("Waiting for Smart Config Create"));
    }
  }
  
  while(!displayConnectionDetails());
  
  // Start multicast DNS responder
  if (!mdns.begin("wildfire", cc3000)) {
    Serial.println(F("Error setting up MDNS responder!"));
    while(1); 
  }
   
  // Start server
  wwwServer.begin();
  
  Serial.println(F("Listening for connections..."));      
}

#define BUFSIZE 511
void loop(void)
{
  char clientline[BUFSIZE];
  int index = 0;  
  
  // Handle any multicast DNS requests
  mdns.update();

  // listen for incoming clients
  WildFire_CC3000_ClientRef client = wwwServer.available();
  if (client) {

    //  reset input buffer
    index = 0;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //  fill url the buffer
        if(c != '\n' && c != '\r' && index < BUFSIZE){ // Reads until either an eol character is reached or the buffer is full
          clientline[index++] = c;
          continue;
        }  

#if DEBUG
        Serial.print("client available bytes before flush: "); Serial.println(client.available());
        Serial.print("request = "); Serial.println(clientline);
#endif

        // Flush any remaining bytes from the client buffer
        //client.flush();

#if DEBUG
        // Should be 0
        Serial.print("client available bytes after flush: "); Serial.println(client.available());
#endif

        //  convert clientline into a proper
        //  string for further processing
        String urlString = String(clientline);

        //  extract the operation
        String op = urlString.substring(0,urlString.indexOf(' '));

        //  we're only interested in the first part...
        urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));

        //  put what's left of the URL back in client line
#if CASESENSE
        urlString.toUpperCase();
#endif
        urlString.toCharArray(clientline, BUFSIZE);

        //  get the first two parameters
        char *pin = strtok(clientline,"/");
        char *value = strtok(NULL,"/");

        //  this is where we actually *do something*!
        char outValue[10] = "MU";
        String jsonOut = String();

        if(pin != NULL){
          if(value != NULL){

#if DEBUG
            //  set the pin value
            Serial.println("setting pin");
#endif

            //  select the pin
            int selectedPin = atoi (pin);
#if DEBUG
            Serial.println(selectedPin);
#endif

            //  set the pin for output
            pinMode(selectedPin, OUTPUT);

            //  determine digital or analog (PWM)
            if(strncmp(value, "HIGH", 4) == 0 || strncmp(value, "LOW", 3) == 0){

#if DEBUG
              //  digital
              Serial.println("digital");
#endif

              if(strncmp(value, "HIGH", 4) == 0){
#if DEBUG
                Serial.println("HIGH");
#endif
                digitalWrite(selectedPin, HIGH);
              }

              if(strncmp(value, "LOW", 3) == 0){
#if DEBUG
                Serial.println("LOW");
#endif
                digitalWrite(selectedPin, LOW);
              }

            } 
            else {

#if DEBUG
              //  analog
              Serial.println("analog");
#endif
              //  get numeric value
              int selectedValue = atoi(value);              
#if DEBUG
              Serial.println(selectedValue);
#endif
              analogWrite(selectedPin, selectedValue);

            }

            //  return status
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();

          } 
          else {
#if DEBUG
            //  read the pin value
            Serial.println("reading pin");
#endif

            //  determine analog or digital
            if(pin[0] == 'a' || pin[0] == 'A'){

              //  analog
              int selectedPin = pin[1] - '0';

#if DEBUG
              Serial.println(selectedPin);
              Serial.println("analog");
#endif

              sprintf(outValue,"%d",analogRead(selectedPin));

#if DEBUG
              Serial.println(outValue);
#endif

            } 
            else if(pin[0] != NULL) {

              //  digital
              int selectedPin = pin[0] - '0';

#if DEBUG
              Serial.println(selectedPin);
              Serial.println("digital");
#endif

              pinMode(selectedPin, INPUT);

              int inValue = digitalRead(selectedPin);

              if(inValue == 0){
                sprintf(outValue,"%s","LOW");
                //sprintf(outValue,"%d",digitalRead(selectedPin));
              }

              if(inValue == 1){
                sprintf(outValue,"%s","HIGH");
              }

#if DEBUG
              Serial.println(outValue);
#endif
            }

            //  assemble the json output
            jsonOut += "{\"";
            jsonOut += pin;
            jsonOut += "\":\"";
            jsonOut += outValue;
            jsonOut += "\"}";

            //  return value with wildcarded Cross-origin policy
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Access-Control-Allow-Origin: *");
            client.println();
            client.println(jsonOut);
          }
        } 
        else {

          //  error
#if DEBUG
          Serial.println("erroring");
#endif
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();

        }
        break;
      }
    }

    // give the web browser time to receive the data
    delay(1000);

    // close the connection:
    client.close();
  }
}

bool displayConnectionDetails(void) {
  uint32_t addr, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&addr, &netmask, &gateway, &dhcpserv, &dnsserv))
    return false;

  Serial.print(F("IP Addr: ")); cc3000.printIPdotsRev(addr);
  Serial.print(F("\r\nNetmask: ")); cc3000.printIPdotsRev(netmask);
  Serial.print(F("\r\nGateway: ")); cc3000.printIPdotsRev(gateway);
  Serial.print(F("\r\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
  Serial.print(F("\r\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
  Serial.println();
  return true;
}

boolean attemptSmartConfigReconnect(void){
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!! Note the additional arguments in .begin that tell the   !!! */
  /* !!! app NOT to deleted previously stored connection details !!! */
  /* !!! and reconnected using the connection details in memory! !!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */  
  if (!cc3000.begin(false, true))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    return false;
  }

  /* Round of applause! */
  Serial.println(F("Reconnected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("\nRequesting DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  return true;
}

boolean attemptSmartConfigCreate(void){
  /* Initialise the module, deleting any existing profile data (which */
  /* is the default behaviour)  */
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin(false))
  {
    return false;
  }  
  
  /* Try to use the smart config app (no AES encryption), saving */
  /* the connection details if we succeed */
  Serial.println(F("Waiting for a SmartConfig connection (~60s) ..."));
  if (!cc3000.startSmartConfig(false))
  {
    Serial.println(F("SmartConfig failed"));
    return false;
  }
  
  Serial.println(F("Saved connection details and connected to AP!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) 
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  
  return true;
}
