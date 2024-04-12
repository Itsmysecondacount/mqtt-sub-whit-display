#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebSrv.h>

// Configuración del servidor MQTT
const char* mqttServer = "192.168.1.40";
const int mqttPort = 1883;
const char* mqttTopic1 = "jamk/tanque/temperatura1";
const char* mqttTopic2 = "jamk/tanque/temperatura2";

// Configuración del cliente WiFi
const char* ssid = "JAMK4";
const char* password = "JAMK12345";
AsyncWebServer server(80);


// Configurar la IP estática
IPAddress ip(192, 168, 1, 36);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Configuración de la luz LED WS2812B
#define CHIPSET     WS2812B
const int numLeds = 6;
const int dataPin = 26;  // Cambia al pin que estás utilizando
CRGB leds[numLeds];
#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60

// Configuración de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables para el manejo de la luz y el parpadeo
bool wifiConfigured = false;
bool toggleState = false;

// Variables para almacenar las temperaturas
float temperature1 = 0.0;
float temperature2 = 0.0;

// Cliente MQTT
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Configurar la luz LED WS2812B
  FastLED.addLeds<CHIPSET, dataPin, GRB>(leds, numLeds).setCorrection( TypicalLEDStrip );

  // Establecer el color inicial a amarillo
  setLEDColor(CRGB::Yellow);

  // Configurar la pantalla OLED
  if(!display.begin(SSD1306_BLACK, 0x3C)) {
    Serial.println(F("No se pudo inicializar la pantalla OLED"));
    while(true);
  }

  // Inicializar la conexión WiFi
  connectWiFi();

  // Inicializar el cliente MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  connectMQTT();

  // Configurar las rutas de la API
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    toggleLED();
    request->send(200, "text/plain", "Toggle LED");
  });

  // Iniciar el servidor HTTP
  server.begin();
}

void loop() {
  // Manejar la conexión WiFi y MQTT
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // Actualizar la pantalla OLED con las temperaturas
  updateOLED();

  // Manejar la luz LED
  if (!wifiConfigured) {
    setLEDColor(CRGB::Yellow);
  }
}

void connectWiFi() {
    // Configurar la IP estática
  WiFi.config(ip, gateway, subnet);
  Serial.println("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando...");
  }
  wifiConfigured = true;
  // Imprimir la dirección IP asignada
  Serial.print("Conectado a la red WiFi. Dirección IP: ");
  Serial.println(WiFi.localIP());

  setLEDColor(CRGB::Black);
}

void connectMQTT() {
  Serial.println("Conectando a MQTT...");
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe(mqttTopic1);
      client.subscribe(mqttTopic2);
      Serial.println("Conectado a MQTT");
    } else {
      Serial.println("Error de conexión a MQTT. Intentando nuevamente en 5 segundos...");
      delay(5000);
    }
  }
}

void updateOLED() {
  // Limpiar la pantalla
  display.clearDisplay();

  // Configurar la fuente y mostrar las temperaturas
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 4);
  display.print(temperature1);
  display.print(" C");

  display.setCursor(0, 28);
  display.print(temperature2);
  display.print(" C");

  // Enviar a la pantalla
  display.display();
}

void setLEDColor(CRGB color) {
  fill_solid(leds, numLeds, color);
  FastLED.show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tópico: ");
  Serial.println(topic);

  // Convertir el payload a una cadena de caracteres (String)
  String payloadStr = "";
  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  // Actualizar las temperaturas según el tópico
  if (strcmp(topic, mqttTopic1) == 0) {
    temperature1 = payloadStr.toFloat();
    Serial.println(temperature1);
  } else if (strcmp(topic, mqttTopic2) == 0) {
    temperature2 = payloadStr.toFloat();
    Serial.println(temperature2);
  }

  // Actualizar la pantalla OLED con las temperaturas
  updateOLED();
}

void toggleLED() {
  // Parpadear el LED con color rojo 5 veces
  for (int i = 0; i < 5; i++) {
    setLEDColor(CRGB::Red);
    FastLED.show();
    delay(500);
    setLEDColor(CRGB::Black);  // Apagar el LED
    FastLED.show();
    delay(500);
  }
}
