/*  Leana Nakkour
 *  HCDE 440 - Assignment 3
 *  April 24, 2019
 * 
 * 
 *  NOTE: Received help from TA, using partial solution code 3.1/3.5 and from ICE #4, and help from Alaa Amed
 */

// This Program incorporates some sensors to build a weather station using the DHT22 and MPL115A2 sensors on a breadboard

//Requisite Libraries . . .
#include <ESP8266WiFi.h> 
#include <SPI.h>
#include <Wire.h>  // for I2C communications
#include <Adafruit_Sensor.h>  // the generic Adafruit sensor library used with both sensors
#include <DHT.h>   // temperature and humidity sensor library
#include <DHT_U.h> // unified DHT library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPL115A2.h> // Barometric pressure sensor library
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //

// pin connected to DH22 data line
#define DATA_PIN 12
// create DHT22 instance
DHT_Unified dht(DATA_PIN, DHT22);
// create MPL115A2 instance
Adafruit_MPL115A2 mpl115a2;


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// digital pin 2
#define BUTTON_PIN 2

// button state
bool current = false;
bool last = false;


#define wifi_ssid "University of Washington"  
#define wifi_password "" 


#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


WiFiClient espClient;             //blah blah blah, espClient
PubSubClient mqtt(espClient);     //blah blah blah, tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array
unsigned long currentMillis = millis();
unsigned long timerOne, timerTwo, timerThree; //we are using these to hold the values of our timers

void setup() {
  // set button pin as an input
  pinMode(BUTTON_PIN, INPUT);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  mpl115a2.begin();
  display.display();
  // start the serial connection
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function

  // Sets each timer to current time
  timerOne = millis();
  timerTwo = millis();
  timerThree = millis();

  // wait for serial monitor to open
  while(! Serial);

  // initialize dht22
  dht.begin();
}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // Start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}  

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("Leana/+"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/////LOOP/////
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  // Read data from sensor: DHT22 values every 3 seconds
  if (millis() - timerOne > 3000) {
    sensors_event_t event;                // Initializes the event (inforamtion read by sensor)
    dht.temperature().getEvent(&event);   // getEvent gets the TEMP information read by the DH22 

    float temp = event.temperature;
    float fahrenheit = (temp * 1.8) + 32;     // Converts temp to fahrenheit
    
    dht.humidity().getEvent(&event);          // gets Humidity "event"
    float hum = event.relative_humidity;
     
    
    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_hum[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character

    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(temp, 5, 2, str_temp);
    //ditto
    dtostrf(hum, 5, 2, str_hum);

    // Publish message to the tem/hum topic 
    sprintf(message, "{\"Temperature\":\"%s\", \"Humidity\":\"%s\"}", str_temp, str_hum); //JSON format using {"XX":"XX"}
    mqtt.publish("Leana/tempHum", message);
    timerOne = millis();

    // Cast all float values into string so it can pe printed on the OLED module
    String temp1 = String(temp); 
    String hum1 = String(hum);
    
    display.clearDisplay();    // Clear the screen
   
    display.setTextSize(1);    // Normal 1:1 pixel scale
   
    display.setCursor(0, 0);   // Start at top-left corner
   
    display.setTextColor(WHITE);  // Draw white text

    display.println("Hi mom, I did it!"); //wholesomeness factor
    display.println("Humidity: " + hum1);   // Display Humidity on OLED

    display.println("Temperature: " + temp1 + " C");      // Display Temperature on OLED
    
    // Update the screen-- without this command, nothing will be pushed/displayed 
    display.display();
    // wait 2 seconds (2000 milliseconds == 2 seconds)
    delay(2000);
    display.clearDisplay();
  }

  // Read data from sensor: MPL115A2 values every 5 seconds
  if (millis() - timerTwo > 5000) {;
    float pressure = 0; 
    pressure = mpl115a2.getPressure();
    sprintf(message, "{\"Pressure\" : \"%d\"}", pressure); // %d is for int
    // Publish message to the pressure topic 
    mqtt.publish("Leana/pressure", message);
    timerTwo = millis();
  }
  // Read data from push button every 3 seconds
  if (millis() - timerThree > 3000) {
    if (digitalRead(BUTTON_PIN) == LOW) {
        current = true;
    } else {
        current = false;
    }
    sprintf(message, "{\"Button State\" : \"%d\"}", current); // %d is used for a bool as well
    mqtt.publish("Leana/switch", message);
    timerThree = millis();
    last = current;
  }
  
}//end Loop

/////CALLBACK/////
//The callback is where we attach a listener to the incoming messages from the server.
//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier . . . (will not appear in separate
//tab on github!)
/////

//////////////// ICE 4 CODE /////////////////////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////

  if (strcmp(topic, "theTopic/LBIL") == 0) {
    Serial.println("A message from Batman . . .");
  }

  else if (strcmp(topic, "theTopic/tempHum") == 0) {
    Serial.println("Some weather info has arrived . . .");
  }

  else if (strcmp(topic, "theTopic/switch") == 0) {
    Serial.println("The switch state is being reported . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
