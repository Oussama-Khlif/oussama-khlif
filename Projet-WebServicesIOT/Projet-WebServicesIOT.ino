#include <WiFi.h>
#include <WiFiUdp.h> // CoAP
#include <coap-simple.h>
#include <DHT.h>
#include <HTTPClient.h> // REST

const char* ssid = "Xiaomi Redmi Note 13 Pro";
const char* password = "12345678";
const char* restEndpoint = "http://192.168.151.40:3000/api/data"; // Address of server.js

#define DHT_PIN 13      // Pin connected to DHT11 sensor
#define DHT_TYPE DHT11  // DHT sensor type

DHT dht(DHT_PIN, DHT_TYPE);

WiFiUDP udp;
Coap coap(udp);

bool LEDSTATE;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  LEDSTATE = true;

  coap.server(callback_light, "light");
  coap.response(callback_response);
  coap.start();
}

void loop() {
  delay(1000);
  coap.loop();

  // Read temperature and humidity from DHT11 sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check if temperature and humidity values are valid
  if (!isnan(temperature) && !isnan(humidity)) {
    // Send data via REST API
    sendRESTData(temperature, humidity);
  } else {
    Serial.println("Failed to read temperature and humidity values from DHT sensor");
  }
}

void callback_light(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Light] ON/OFF");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  String message(p);

  if (message.equals("0"))
    LEDSTATE = false; // Turn off
  else if (message.equals("1"))
    LEDSTATE = true; // Turn on

  if (LEDSTATE) {
    digitalWrite(14, HIGH);
    coap.sendResponse(ip, port, packet.messageid, "1");
  } else {
    digitalWrite(14, LOW);
    coap.sendResponse(ip, port, packet.messageid, "0");
  }
}

void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[CoAP Response received]");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  Serial.println(p);
}

void sendRESTData(float temperature, float humidity) {
  HTTPClient http;

  // Send data in plain text format
  String dataPayload = String(temperature) + "," + String(humidity);
  Serial.println("Sending plain text payload: " + dataPayload);

  http.begin(restEndpoint);
  http.addHeader("Content-Type", "text/plain"); // Set content type to text/plain
  int httpResponseCode = http.POST(dataPayload);

  if (httpResponseCode > 0) {
    Serial.printf("HTTP POST request sent with status code: %d\n", httpResponseCode);
  } else {
    Serial.printf("HTTP POST request failed with error: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}
