#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ===== CẤU HÌNH WiFi & MQTT =====
const char* ssid = "Hieu Cute";        // Thay bằng WiFi của bạn
const char* password = "ximon123"; // Thay bằng mật khẩu WiFi
const char* mqtt_server = "broker.emqx.io";  // EMQX broker
const int mqtt_port = 1883;

// ===== MQTT TOPICS =====
#define TOPIC_SENSOR_GAS "smartcity/sensor/gas"
#define TOPIC_SENSOR_SOUND "smartcity/sensor/sound"
#define TOPIC_CONTROL_BUZZER "smartcity/control/buzzer"
#define TOPIC_CONTROL_LED "smartcity/control/led"
#define TOPIC_CONTROL_MODE "smartcity/control/mode"
#define TOPIC_OLED_MESSAGE "smartcity/oled/message"
#define TOPIC_STATUS_BUZZER "smartcity/status/buzzer"  // Thêm topic trạng thái còi
#define TOPIC_STATUS_LED "smartcity/status/led"       // Thêm topic trạng thái LED

// ===== OLED CONFIG =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== PIN DEFINITIONS =====
#define MQ2_DIGITAL_PIN 16    // D0 - GPIO16
#define BUZZER_PIN 14         // D5 - GPIO14  
#define MAX9814_PIN A0        // Analog pin A0
#define LED_PIN 15            // D8 - GPIO15
#define OLED_SDA 4            // GPIO4
#define OLED_SCL 5            // GPIO5

// ===== VARIABLES =====
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastUpdate = 0;
unsigned long lastStatusPublish = 0;
String userMessage = "";
bool customMessage = false;
bool manualMode = false;  // false: auto, true: manual
bool manualBuzzerState = false;
bool manualLedState = false;

// Cảm biến
int lastGasState = 1;
int currentSoundValue = 0;

// Trạng thái thực tế của thiết bị
bool actualBuzzerState = false;
bool actualLedState = false;

// ===== FUNCTION PROTOTYPES =====
void displayCenteredText(String line1, String line2, String line3);
void displayCenteredText1(String line1);
void displayCenteredText2(String line1, String line2);
void callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void sendSensorData();
void publishDeviceStatus();

// ===== DISPLAY FUNCTION =====
void displayCenteredText(String line1, String line2, String line3) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  int16_t x1, y1;
  uint16_t w, h;
  
  if(line1.length() > 0) {
    display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 8);
    display.println(line1);
  }
  
  if(line2.length() > 0) {
    display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 32);
    display.println(line2);
  }
  
  if(line3.length() > 0) {
    display.setTextSize(1);
    display.getTextBounds(line3, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 54);
    display.println(line3);
  }
  
  display.display();
}

void displayCenteredText1(String line1) {
  displayCenteredText(line1, "", "");
}

void displayCenteredText2(String line1, String line2) {
  displayCenteredText(line1, line2, "");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(MQ2_DIGITAL_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize I2C & OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED allocation failed!");
  } else {
    displayCenteredText2("SmartCity", "Connecting...");
  }
  
  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  displayCenteredText2("SmartCity", "MQTT Ready");
  delay(2000);
  displayCenteredText2("Welcome to", "Smartcity");
}

// ===== MQTT CALLBACK =====
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Điều khiển chế độ
  if (strcmp(topic, TOPIC_CONTROL_MODE) == 0) {
    manualMode = (message == "manual");
    Serial.print("Mode changed to: ");
    Serial.println(manualMode ? "Manual" : "Auto");
  }
  // Điều khiển còi Manual
  else if (strcmp(topic, TOPIC_CONTROL_BUZZER) == 0 && manualMode) {
    manualBuzzerState = (message == "ON");
    actualBuzzerState = manualBuzzerState;
    digitalWrite(BUZZER_PIN, actualBuzzerState ? HIGH : LOW);
    publishDeviceStatus();  // Gửi trạng thái ngay khi thay đổi
    Serial.print("Manual Buzzer: ");
    Serial.println(actualBuzzerState ? "ON" : "OFF");
  }
  // Điều khiển LED Manual
  else if (strcmp(topic, TOPIC_CONTROL_LED) == 0 && manualMode) {
    manualLedState = (message == "ON");
    actualLedState = manualLedState;
    digitalWrite(LED_PIN, actualLedState ? HIGH : LOW);
    publishDeviceStatus();  // Gửi trạng thái ngay khi thay đổi
    Serial.print("Manual LED: ");
    Serial.println(actualLedState ? "ON" : "OFF");
  }
  // Hiển thị OLED message
  else if (strcmp(topic, TOPIC_OLED_MESSAGE) == 0) {
    if (message == "0") {
      customMessage = false;
      if (!manualMode) displayCenteredText2("Welcome to", "Smartcity");
    } else {
      customMessage = true;
      userMessage = message;
      if (userMessage.length() > 12) {
        String line1 = userMessage.substring(0, 12);
        String line2 = userMessage.substring(12, 24);
        displayCenteredText2(line1, line2);
      } else {
        displayCenteredText1(userMessage);
      }
    }
  }
}

// ===== MQTT RECONNECT =====
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP8266_SmartCity")) {
      Serial.println("connected");
      client.subscribe(TOPIC_CONTROL_MODE);
      client.subscribe(TOPIC_CONTROL_BUZZER);
      client.subscribe(TOPIC_CONTROL_LED);
      client.subscribe(TOPIC_OLED_MESSAGE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// ===== PUBLISH DEVICE STATUS =====
void publishDeviceStatus() {
  // Gửi trạng thái còi
  String buzzerStatus = actualBuzzerState ? "ON" : "OFF";
  client.publish(TOPIC_STATUS_BUZZER, buzzerStatus.c_str());
  
  // Gửi trạng thái LED
  String ledStatus = actualLedState ? "ON" : "OFF";
  client.publish(TOPIC_STATUS_LED, ledStatus.c_str());
  
  Serial.print("Published status - Buzzer: ");
  Serial.print(buzzerStatus);
  Serial.print(", LED: ");
  Serial.println(ledStatus);
}

// ===== SEND SENSOR DATA =====
void sendSensorData() {
  // Gửi dữ liệu khí gas
  String gasStatus = (lastGasState == 0) ? "DANGER" : "SAFE";
  client.publish(TOPIC_SENSOR_GAS, gasStatus.c_str());
  
  // Gửi dữ liệu âm thanh
  char soundStr[10];
  itoa(currentSoundValue, soundStr, 10);
  client.publish(TOPIC_SENSOR_SOUND, soundStr);
}

// ===== MAIN LOOP =====
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  unsigned long currentMillis = millis();
  
  // Cập nhật cảm biến mỗi 1 giây
  if (currentMillis - lastUpdate >= 1000) {
    lastUpdate = currentMillis;
    
    // Đọc cảm biến khí gas
    int gasState = digitalRead(MQ2_DIGITAL_PIN);
    lastGasState = gasState;
    
    // Đọc cảm biến âm thanh
    currentSoundValue = analogRead(MAX9814_PIN);
    
    // Chế độ AUTO
    if (!manualMode) {
      // Auto control buzzer
      if (gasState == 0) {
        actualBuzzerState = true;
        digitalWrite(BUZZER_PIN, HIGH);
        if (!customMessage) {
          displayCenteredText2("GAS ALERT!", "NOT SAFE!");
        }
      } else {
        actualBuzzerState = false;
        digitalWrite(BUZZER_PIN, LOW);
        if (!customMessage) {
          displayCenteredText2("Welcome to", "Smartcity");
        }
      }
      
      // Auto control LED (âm thanh > 600)
      if (currentSoundValue > 600) {
        actualLedState = true;
        digitalWrite(LED_PIN, HIGH);
      } else {
        actualLedState = false;
        digitalWrite(LED_PIN, LOW);
      }
    }
    
    // Gửi dữ liệu cảm biến lên MQTT
    sendSensorData();
    
    // Log Serial
    Serial.print("Gas: ");
    Serial.print(gasState == 0 ? "DANGER" : "SAFE");
    Serial.print(" | Sound: ");
    Serial.print(currentSoundValue);
    Serial.print(" | Mode: ");
    Serial.print(manualMode ? "Manual" : "Auto");
    Serial.print(" | Buzzer: ");
    Serial.print(actualBuzzerState ? "ON" : "OFF");
    Serial.print(" | LED: ");
    Serial.println(actualLedState ? "ON" : "OFF");
  }
  
  // Gửi trạng thái thiết bị mỗi 2 giây (để đảm bảo web luôn cập nhật)
  if (currentMillis - lastStatusPublish >= 2000) {
    lastStatusPublish = currentMillis;
    publishDeviceStatus();
  }
}