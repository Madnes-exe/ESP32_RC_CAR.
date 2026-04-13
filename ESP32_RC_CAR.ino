// RC CAR XIAO ESP32-S3 SENSE

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include "webpage.h"

// Konfiguracja kamery
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// Ustawienia WI-FI 
const char* ssid = "ESP32-CAM";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

// Ustawienia pinów oraz wartości dla: MG90S, L298N, PWM 
Servo servo;
const int servoPin = 4;
const int SERVO_LEFT = 0;    //wartości skrętu serwomechanizmu zostały dostowowane do ograniczeń fizycznych układu mechanicznego
const int SERVO_CENTER = 45; 
const int SERVO_RIGHT = 85;  
const int ENA = 1;  
const int IN1 = 2;  
const int IN2 = 3;  
const int pwmFreq = 1000;    
const int pwmResolution = 8; 
const int PWM_START_POWER = 90; //minimalna wartość dla której silnik jest w stanie ruszyć z miejsca (napięcie 3.3V)
const int PWM_MAX = 255;  

// Zmienne FreeRTOS i Failsafe 
SemaphoreHandle_t dataMutex;
int currentSteering = 90;
int currentSpeed = 0;
unsigned long lastCommandTime = 0;
const int TIMEOUT_MS = 500; 

// Zadanie 1: Sterowanie silnikiem i serwomechanizmem
void motorTask(void *pvParameters) {
  for (;;) {
    int localSteering, localSpeed;
    unsigned long localTime;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    localSteering = currentSteering;
    localSpeed = currentSpeed;
    localTime = lastCommandTime;
    xSemaphoreGive(dataMutex);

    if (millis() - localTime > TIMEOUT_MS) {
      localSpeed = 0; 
    }

    int servoAngle = SERVO_CENTER; 
    if (localSteering < 90) {
      servoAngle = map(localSteering, 0, 90, SERVO_LEFT, SERVO_CENTER);
    } else {
      servoAngle = map(localSteering, 90, 180, SERVO_CENTER, SERVO_RIGHT);
    }
    servoAngle = constrain(servoAngle, min(SERVO_LEFT, SERVO_RIGHT), max(SERVO_LEFT, SERVO_RIGHT));
    servo.write(servoAngle);

    int pwmValue = 0;
    int deadzone = 25; 

    if (localSpeed > deadzone) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      pwmValue = map(localSpeed, deadzone, 255, PWM_START_POWER, PWM_MAX);
    } 
    else if (localSpeed < -deadzone) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      pwmValue = map(abs(localSpeed), deadzone, 255, PWM_START_POWER, PWM_MAX);
    } 
    else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      pwmValue = 0;
    }
    ledcWrite(ENA, pwmValue);

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Zadanie 2: Obsługa WebSocket 
void webSocketTask(void *pvParameters) {
  for (;;) {
    webSocket.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    StaticJsonDocument<200> doc;
    if (deserializeJson(doc, payload)) return;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    currentSteering = doc["steering"] | 90;
    currentSpeed = doc["speed"] | 0;
    lastCommandTime = millis();
    xSemaphoreGive(dataMutex);
  }
}

// Strumieniowanie Kamery
void handleStream() {
  WiFiClient client = server.client();
  client.setNoDelay(true);

  String response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

  server.sendContent(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      delay(100);
      continue;
    }

    client.printf("--frame\r\n");
    client.printf("Content-Type: image/jpeg\r\n");
    client.printf("Content-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.printf("\r\n");

    esp_camera_fb_return(fb);

    if (!client.connected()) break;
    delay(40);
  }
}
 
void setup() {
  Serial.begin(115200);

  dataMutex = xSemaphoreCreateMutex();
  lastCommandTime = millis();
// Konfiguracaj ustawień kamery
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000; 
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = psramFound() ? 2 : 1; 
  config.grab_mode = CAMERA_GRAB_LATEST; 
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_camera_init(&config);

  servo.setPeriodHertz(50);
  servo.attach(servoPin, 500, 2400);
  servo.write(SERVO_CENTER);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcAttach(ENA, pwmFreq, pwmResolution);
  ledcWrite(ENA, 0);

  xTaskCreatePinnedToCore(motorTask, "MotorTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(webSocketTask, "WSTask", 4096, NULL, 2, NULL, 1);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.setSleep(false);

  server.on("/", []() { server.send_P(200, "text/html", webpage); });
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
}
