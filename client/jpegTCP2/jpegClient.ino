#pragma GCC push_options
#pragma GCC optimize ("O3") // O3 boosts fps by 20%

#include "JPEGDEC.h"
#include <TFT_eSPI.h>
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();
WiFiClient client;
JPEGDEC jpeg;
const char* SSID = "";
const char* PASSWORD = "";
const int PORT = 5451;
const char* HOST = ""; // host ip address 
const int bufferSize = 50000; // buffer can be smaller as each frame is only around 3.5kb at 50 jpeg quality
uint8_t *buffer;
int bufferLength = 0;
const byte requestMessage[] = {0x55, 0x44, 0x55, 0x11}; // request message should probably be longer to avoid a false positive
unsigned long lastUpdate = 0;
int updates = 0;
//unsigned long lastRequestSent = 0;

int drawJpeg(JPEGDRAW *pDraw) {
  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  return 1;
}

void setup() {
  buffer = (uint8_t*)malloc(bufferSize * sizeof(uint8_t));
  Serial.begin(9600); // fps is reported over serial
  
  tft.init();
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.println("Connecting to Wi-Fi");
  tft.setTextColor(TFT_WHITE);
  tft.println(SSID);
  client = WiFi.begin(SSID, PASSWORD); // TODO: allow device to reconnect if connection is lost

  while (WiFi.status() != WL_CONNECTED) { }
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.println("Wi-Fi connected");

  if(client.connect(HOST, PORT)) {
    tft.println("TCP connected");
  } else {
    tft.setTextColor(TFT_RED);
    tft.println("TCP FAILED");
  }

  lastUpdate = millis();
}

void loop() {
  int dataAvailible = client.available();

  if(dataAvailible > 0) {
    client.read((uint8_t*)&buffer[bufferLength], dataAvailible); // pointer arithmetic is probably bad practice but i'm mainly a JS dev so hacky shit comes naturally to me
    bufferLength += dataAvailible;
  } /*else {
    if(lastRequestSent > 0 && millis() + 10000 > lastRequestSent) {
      client.write(requestMessage, 4); // request new image after timeout
      lastRequestSent = millis();
    }
  }*/

  for(int i = bufferLength; i > 0; --i) { // count backwards from data buffer to search for footer. there's probably a better way
    if(buffer[i - 3] == 0x55 && buffer[i - 2] == 0x44 && buffer[i - 1] == 0x55 && buffer[i] == 0x11) { // more elegent solution required. if multiple images end up in the buffer the jpeg will likely fail to decode
      int dataLength = i - 4;
      //Serial.println(dataLength);
      bufferLength = 0;

      client.write(requestMessage, 4); // queue up a new frame before decoding the current one
      //lastRequestSent = millis();

      if(jpeg.openRAM(buffer, dataLength, drawJpeg)) { // TODO: multithreaded decoding. this would probably be a great boost to performance
        jpeg.setPixelType(RGB565_BIG_ENDIAN);

        if(!jpeg.decode(0, 0, 1)) {
          Serial.println("Could not decode jpeg");
          jpeg.close();
        }
        jpeg.close();
      } else {
        Serial.println("Could not open jpeg");
        jpeg.close();
      }

      updates += 1;
      break;
    }
  }

  unsigned long time = millis();
  
  if(time > lastUpdate + 1000) {
    lastUpdate = time;
    Serial.print("FPS: ");
    Serial.println(updates);
    updates = 0;
  }
}

#pragma GCC pop_options
