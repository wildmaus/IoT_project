#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#define LED_COUNT 64
#define PIN D8
#define CHANGE_MODE D2
#define ENC_A D3
#define ENC_B D5
#define CHANGE_STATE D4
#define START_SID "Esp8266 setup"
#define START_PASS "uLwChk2Z"

void ICACHE_RAM_ATTR change_mode();
void ICACHE_RAM_ATTR change_bright();
void ICACHE_RAM_ATTR change_state();
// mqtt
const char* mqtt_server = "mqtt_server";
const char* client_id = "client_id";
const int mqtt_port = 1883;
WiFiClient myClient;
PubSubClient client(myClient);
// local variables
int curr_mode = 0; // for change mode
int bright = 30; // for change bright
boolean b_or_c = true;
int tmp = 0;
int hsv = 255; // for change color
volatile boolean state0, lastState;
CRGB leds[LED_COUNT];

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
//  wm.resetSettings();
  boolean res;
  res = wm.autoConnect(START_SID, START_PASS); // password protected ap
  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(MQTTcallback); 
  while (!client.connected()) 
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect(client_id))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  client.subscribe("esp/test");
  pinMode(CHANGE_MODE, INPUT);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  pinMode(CHANGE_STATE, INPUT);
  FastLED.addLeds<WS2812, PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(bright);
  attachInterrupt(digitalPinToInterrupt(CHANGE_MODE), change_mode, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A), change_bright, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CHANGE_STATE), change_state, RISING);
}
 
void loop() {
  switch (curr_mode) {
    case 0: turn_off(); break;
    case 1: one_color_all(hsv); break;
    case 2: rainbow(); break;
    case 3: random_light(); break;
    case 4: conf(); break;
    case 5: cicle(); break;
    case 6: focus(); break;
    case 7: rainbow_blink(); break;
  }
  client.loop();
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  String message;
  String number;
  boolean k = false;
  for (int i = 0; i < length; i++) {
    if (k) {
      number = number + (char)payload[i];
    }
    if (String((char)payload[i]) == " ") {
      k = true;
    }
    if (!k) {
      message = message + (char)payload[i];
    }
  }
  if (message == "m") {
    curr_mode = number.toInt();
  }
  if (message == "b") {
    bright = number.toInt();
  }
  if (message == "c") {
    hsv = number.toInt();
  }
}

void change_mode() {
  curr_mode++;
  if (curr_mode > 7) {
    curr_mode = 0;
  }
  Serial.println(curr_mode);
}

void change_bright() {
  if (b_or_c) {
    state0 = digitalRead(ENC_A);
    if (state0 != lastState) {
        bright += (digitalRead(ENC_B) != lastState) ? -5 : 5;
    }
    if (bright > 255) {
      bright = 255;
    }
    if (bright < 0) {
      bright = 0;
    }
    FastLED.setBrightness(bright);
    lastState = state0;
  }
  else {
    state0 = digitalRead(ENC_A);
    if (state0 != lastState) {
        hsv += (digitalRead(ENC_B) != lastState) ? -1 : 1;
    }
    if (hsv > 255) {
      hsv = 0;
    }
    if (hsv < 0) {
      hsv = 255;
    }
    lastState = state0;
  }
}

void change_state() {
  b_or_c = !b_or_c;
  Serial.println("change");
}

void turn_off() {
  FastLED.clear();
  FastLED.show();
}

void one_color_all(int ihsv) {
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i] = CHSV(ihsv, 255, 255);
  }
  FastLED.setBrightness(bright);
  FastLED.show();
}

void rainbow() {
    for (int i = 0; i < LED_COUNT; i++) {
      leds[i] = CHSV(tmp+ i * 5, 255, 255);
    }
    tmp++;
    if (tmp >= 255) {
      tmp = 0;
    }
    FastLED.setBrightness(bright);
    FastLED.show();
    delay(20);
}

void random_light() {
    fadeToBlackBy(leds, LED_COUNT, 2);
    int red = random(0, 255);
    int grn = random(0, 255);
    int blu = random(0, 255);
    int pos = random(0, LED_COUNT);
    leds[pos] = CHSV(red, grn, blu);
    FastLED.setBrightness(bright);
    FastLED.show();
}

void conf() {
    fadeToBlackBy(leds, LED_COUNT, 2);
    int pos = beatsin16(13, 0, LED_COUNT - 1);
    tmp++;    
    if (tmp >= 255) {
      tmp = 0;
    }
    leds[pos] += CHSV(tmp, 255, 192);
    FastLED.setBrightness(bright);
    FastLED.show();
} 

void cicle() {
    for (int i = 0; i < LED_COUNT; i++) {
      leds[i].nscale8(250);
    }
    tmp++;    
    if (tmp >= 255) {
      tmp = 0;
    }
    for (int i = 0; i < LED_COUNT; i++) {
      leds[i] = CHSV(tmp, 255, 255);
      FastLED.setBrightness(bright);
      FastLED.show();
    }
}

void focus() {
    fadeToBlackBy(leds, LED_COUNT, 2);   
    tmp++;    
    if (tmp >= 255) {
      tmp = 0;
    }
    for (int i = 0; i < 8; i++) {
      leds[beatsin16(i + 7, 0, LED_COUNT - 1)] |= CHSV(tmp+=16, 200, 255);
    }
    FastLED.setBrightness(bright);
    FastLED.show();
    delay(20);
}

void rainbow_blink() {
    tmp++;    
    if (tmp >= 255) {
      tmp = 0;
    }
    fill_rainbow( leds, LED_COUNT, tmp, 7);
    if (random8() < 80) { leds[ random16(LED_COUNT) ] += CRGB::White; }
    FastLED.setBrightness(bright);
    FastLED.show();
    delay(20);
}
