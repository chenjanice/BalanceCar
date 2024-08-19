#include <esp_now.h>
#include <WiFi.h>
#include <M5StickCPlus.h>


#define GRAPH_WIDTH 125
#define GRAPH_HEIGHT 90
#define GRAPH_X 5
#define GRAPH_Y 20
#define UPDATE_INTERVAL 500
#define MAX_DATA_POINTS 25 // 125 / 5 = 25 數據點

uint8_t broadcastAddress[] = {0x4C, 0x75, 0x25, 0xAD, 0x29, 0x68};

float gyroXdata[MAX_DATA_POINTS] = {0};
float gyroYdata[MAX_DATA_POINTS] = {0};
float gyroZdata[MAX_DATA_POINTS] = {0};
float accXdata[MAX_DATA_POINTS] = {0};
float accZdata[MAX_DATA_POINTS] = {0};

int dataIndex = 0;
unsigned long lastUpdateTime = 0;


typedef struct struct_message {
    float gyroXdata;
    float gyroYdata;
    float gyroZdata;
    float accXdata;
    float accZdata;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void drawGraphBackground() {
  M5.Lcd.drawRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, WHITE);
  M5.Lcd.drawLine(GRAPH_X, GRAPH_Y + GRAPH_HEIGHT/2, GRAPH_X + GRAPH_WIDTH, GRAPH_Y + GRAPH_HEIGHT/2, DARKGREY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(5, 5);
}

void updateData() {
  float gX, gY, gZ, aX, aY, aZ;
  M5.Imu.getGyroData(&gX, &gY, &gZ);
  M5.Imu.getAccelData(&aX, &aY, &aZ);

  gyroXdata[dataIndex] = constrain(-gZ / 250.0, -1, 1);
  gyroYdata[dataIndex] = constrain(gX / 250.0, -1, 1);
  gyroZdata[dataIndex] = constrain(-gY / 250.0, -1, 1);
  accXdata[dataIndex] = constrain(aZ, -1, 1);
  accZdata[dataIndex] = constrain(aY, -1, 1);

  dataIndex = (dataIndex + 1) % MAX_DATA_POINTS; 
}

void drawLegend() {
  const int legendY = GRAPH_Y + GRAPH_HEIGHT + 5;
  const int textSpacing = 15;
  const int textWidth = 130;
  int latestIndex = (dataIndex - 1 + MAX_DATA_POINTS) % MAX_DATA_POINTS;

  M5.Lcd.fillRect(GRAPH_X, legendY, textWidth, textSpacing * 6, BLACK);
  
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(GRAPH_X ,legendY + textSpacing*1);
  M5.Lcd.printf("gX:%.2f", gyroXdata[latestIndex]);

  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(GRAPH_X , legendY + textSpacing*2);
  M5.Lcd.printf("gY:%.2f", gyroYdata[latestIndex]);

  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(GRAPH_X , legendY + textSpacing*3);
  M5.Lcd.printf("gZ:%.2f", gyroZdata[latestIndex]);

  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(GRAPH_X , legendY + textSpacing*4);
  M5.Lcd.printf("aX:%.2f", accXdata[latestIndex]);

  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.setCursor(GRAPH_X , legendY + textSpacing*5);
  M5.Lcd.printf("aZ:%.2f", accZdata[latestIndex]);
}


void drawGraph() {
  M5.Lcd.fillRect(GRAPH_X + 1, GRAPH_Y + 1, GRAPH_WIDTH - 2, GRAPH_HEIGHT - 2, BLACK);

  for (int i = 0; i < MAX_DATA_POINTS - 1; i++) {
    int index = (dataIndex + i) % MAX_DATA_POINTS;
    int nextIndex = (dataIndex + i + 1) % MAX_DATA_POINTS;

    drawLine(i, gyroXdata[index], gyroXdata[nextIndex], RED);
    drawLine(i, gyroYdata[index], gyroYdata[nextIndex], GREEN);
    drawLine(i, gyroZdata[index], gyroZdata[nextIndex], BLUE);
    drawLine(i, accXdata[index], accXdata[nextIndex], YELLOW);
    drawLine(i, accZdata[index], accZdata[nextIndex], MAGENTA);
  }
  drawLegend();
}

void drawLine(int x, float y1, float y2, uint16_t color) {
  int x1 = GRAPH_X + x * 5;
  int x2 = GRAPH_X + (x + 1) * 5;
  int y1Mapped = map(y1 * 1000, -1000, 1000, GRAPH_Y + GRAPH_HEIGHT - 1, GRAPH_Y + 1);
  int y2Mapped = map(y2 * 1000, -1000, 1000, GRAPH_Y + GRAPH_HEIGHT - 1, GRAPH_Y + 1);
  M5.Lcd.drawLine(x1, y1Mapped, x2, y2Mapped, color);
}

void initializeDataArrays() {
  for (int i = 0; i < MAX_DATA_POINTS; i++) {
    gyroXdata[i] = 0.0f;
    gyroYdata[i] = 0.0f;
    gyroZdata[i] = 0.0f;
    accXdata[i] = 0.0f;
    accZdata[i] = 0.0f;
  }
}

void displayBatteryLevel() {
  float batteryVoltage = M5.Axp.GetBatVoltage();
  int batteryPercentage = mapBatteryPercentage(batteryVoltage);
  M5.Lcd.fillRect(0, 0, GRAPH_WIDTH - 2, 20, BLACK);
  M5.Lcd.setCursor(5, 5);
  M5.Lcd.printf("Battery: %d%% (%.2fV)", batteryPercentage, batteryVoltage);
}

int mapBatteryPercentage(float voltage) {
  if (voltage > 4.2f) voltage = 4.2f;
  if (voltage < 3.0f) voltage = 3.0f;
  int percentage = (voltage - 3.0f) / (4.2f - 3.0f) * 100.0f;
  return percentage;
}

void setup() {
  M5.begin();
  M5.Imu.Init();
  M5.Lcd.setRotation(2);  // USB 在頂部
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  drawGraphBackground();
  initializeDataArrays();

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = currentTime;
    updateData();
    int latestIndex = (dataIndex - 1 + MAX_DATA_POINTS) % MAX_DATA_POINTS;

    myData.gyroXdata = gyroXdata[latestIndex];
    myData.gyroYdata = gyroYdata[latestIndex];
    myData.gyroZdata = gyroZdata[latestIndex];
    myData.accXdata = accXdata[latestIndex];
    myData.accZdata = accZdata[latestIndex];
    
    drawGraph();
    displayBatteryLevel();

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else { 
    Serial.println("Error sending the data");
  }
  }
}