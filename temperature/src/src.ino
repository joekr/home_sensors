#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

#include "config.h"

void handle_http_metrics_client();
void handle_http_metrics_client();

ESP8266WebServer http_server(HTTP_SERVER_PORT);
DHT dht(DHTPIN, DHTTYPE);

float humidity, temperature, heat_index;

void connect(){
  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  connect();
  setup_http_server();
  dht.begin();
}

// the loop function runs over and over again forever
void loop() {
  http_server.handleClient();
}

void setup_http_server() {
    http_server.on("/", HTTPMethod::HTTP_GET, handle_http_home_client);
    http_server.on(HTTP_METRICS_ENDPOINT, HTTPMethod::HTTP_GET, handle_http_metrics_client);
    http_server.begin();
    Serial.println("HTTP server started.");
    char message[128];
    snprintf(message, 128, "Metrics endpoint: %s", HTTP_METRICS_ENDPOINT);
    Serial.println(message);
}

void handle_http_home_client() {
    static size_t const BUFSIZE = 256;
    static char const *response_template =
        "Prometheus ESP8266 DHT Exporter.\n"
        "\n"
        "Usage: %s\n";
    char response[BUFSIZE];
    snprintf(response, BUFSIZE, response_template, HTTP_METRICS_ENDPOINT);
    http_server.send(200, "text/plain; charset=utf-8", response);
}

void handle_http_metrics_client() {
    static size_t const BUFSIZE = 1024;
    static char const *response_template =
        "# HELP iot_info Metadata about the device.\n"
        "# TYPE iot_info gauge\n"
        "# UNIT iot_info \n"
        "iot_info{version=\"%s\",room=\"%s\",board=\"%s\",sensor=\"%s\"} 1\n"
        "# HELP iot_air_humidity_percent Air humidity.\n"
        "# TYPE iot_air_humidity_percent gauge\n"
        "# UNIT iot_air_humidity_percent %%\n"
        "iot_air_humidity_percent{room=\"%s\"} %f\n"
        "# HELP iot_air_temperature_fahrenheit Air temperature.\n"
        "# TYPE iot_air_temperature_fahrenheit gauge\n"
        "# UNIT iot_air_temperature_fahrenheit \u00B0C\n"
        "iot_air_temperature_fahrenheit{room=\"%s\"} %f\n"
        "# HELP iot_air_heat_index_fahrenheit Apparent air temperature, based on temperature and humidity.\n"
        "# TYPE iot_air_heat_index_fahrenheit gauge\n"
        "# UNIT iot_air_heat_index_fahrenheit \u00B0C\n"
        "iot_air_heat_index_fahrenheit{room=\"%s\"} %f\n";

    read_sensor();
    if (isnan(humidity) || isnan(temperature) || isnan(heat_index)) {
        http_server.send(500, "text/plain; charset=utf-8", "Sensor error.");
        return;
    }

    char response[BUFSIZE];
    snprintf(response, BUFSIZE, response_template, VERSION, ROOM, BOARD_NAME, DHT_NAME, ROOM, humidity, ROOM, temperature, ROOM, heat_index);
    http_server.send(200, "text/plain; charset=utf-8", response);
}

void read_sensor() {

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  humidity = h;
  temperature = f;
  heat_index = hif;

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
}

