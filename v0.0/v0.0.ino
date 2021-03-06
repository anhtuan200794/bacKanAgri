#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "DHT.h"

#define WIFI_AP "TP-LINK_D2A1EA"            // tên mạng wifi
#define WIFI_PASSWORD "244466666@2017"   // mật khẩu
#define CLIENTID "5a5643736c8e6e04bcefde99" // vuon 1
//#define CLIENTID "5a579a6b85520f07df6347e9"  //vuon 2
#define DHTPIN 5
#define DHTTYPE DHT11

#define D1 4
#define D2 14
#define D3 12
//#define D4 13
//#define LED_BUILTIN 2

//#define GPIO0_PIN 3
// #define GPIO2_PIN 5

DHT dht(DHTPIN, DHTTYPE);
char thingsboardServer[] = "thienha.net";

WiFiClient wifiClient;

PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
unsigned long lastSend = 0;
// We assume that all GPIOs are LOW
boolean gpioState[] = {false, false};

void setup() {
  Serial.begin(115200);
  delay(10);
  InitWiFi();
  dht.begin();
  client.setServer( thingsboardServer, 1883 );
  client.setCallback(on_message);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  // pinMode(D4, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(D1, HIGH);
  digitalWrite(D2, HIGH);
  digitalWrite(D3, HIGH);
  //digitalWrite(D4, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

}

void loop() {
  if ( !client.connected() ) {
    digitalWrite(LED_BUILTIN, LOW);
    reconnect();
  }
  digitalWrite(LED_BUILTIN, HIGH);
  if ( millis() - lastSend > 5000 ) { // Update and send only after 1 seconds
    getAndSendTemperatureAndHumidityData();
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    lastSend = millis();
  }
  client.loop();
}

void getAndSendTemperatureAndHumidityData()
{
  Serial.println("Collecting temperature data.");

  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");

  String temperature = String(t);
  String humidity = String(h);

  // Just debug messages
  //  Serial.println( "Sending temperature and humidity : [" );
  //  Serial.print( temperature ); Serial.print( "," );
  //  Serial.print( humidity );
  //  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;
  payload += "}";

  // Send payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "admin/attributes", attributes );
  Serial.println( attributes );
}

// The callback for when a PUBLISH message is received from the server.
void on_message(const char* topic, byte* payload, unsigned int length) {

  Serial.println("On message");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  // Decode JSON request
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  set_gpio_status(data["device1"], data["device2"], data["device3"]);
  client.publish("admin/response", get_gpio_status().c_str());
}

String get_gpio_status() {
  // Prepare gpios JSON payload string
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();
  data["device1"] = gpioState[0] ? true : false;
  data["device2"] = gpioState[1] ? true : false;
  data["device3"] = gpioState[2] ? true : false;
  //data["device4"] = gpioState[2] ? true : false;
  char payload[256];
  data.printTo(payload, sizeof(payload));
  String strPayload = String(payload);
  Serial.print("Get gpio status: ");
  Serial.println(strPayload);
  return strPayload;
}

void set_gpio_status(bool dv1, bool dv2, bool dv3) {
  if (dv1) {
    // Output GPIOs state
    digitalWrite(D1, LOW);
    // Update GPIOs state
    gpioState[0] = dv1;
  } else {
    digitalWrite(D1, HIGH);
    // Update GPIOs state
    gpioState[0] = dv1;
  }

  if (dv2) {
    // Output GPIOs state
    digitalWrite(D2, LOW);
    // Update GPIOs state
    gpioState[1] = dv2;
  } else {
    digitalWrite(D2, HIGH);
    // Update GPIOs state
    gpioState[1] = dv2;
  }

  if (dv3) {
    // Output GPIOs state
    digitalWrite(D3, LOW);
    // Update GPIOs state
    gpioState[2] = dv3;
  } else {
    digitalWrite(D3, HIGH);
    // Update GPIOs state
    gpioState[2] = dv3;
  }
  //  if (dv4) {
  //    // Output GPIOs state
  //    digitalWrite(D4, LOW);
  //    // Update GPIOs state
  //    gpioState[3] = dv4;
  //  } else {
  //    digitalWrite(D4, HIGH);
  //    // Update GPIOs state
  //    gpioState[3] = dv4;
  //    }
}

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to Server node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(CLIENTID, "admin", NULL) ) {
      Serial.println( "[DONE]" );

      // Subscribing to receive RPC requests
      Serial.println("Subcribe !");
      client.subscribe("admin/request");
      // Sending current GPIO status
      //Serial.println("Sending current GPIO status ...");
      //client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      digitalWrite(LED_BUILTIN, LOW);
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

