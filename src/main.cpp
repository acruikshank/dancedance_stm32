#include <Arduino.h>
#include <WS2812B.h>

#define NUM_LEDS 50
#define EN1 PB9
#define EN2 PB4
#define EN3 PA2
#define EN4 PA3
#define EN5 PB7
#define EN6 PB6
#define EN7 PB5
#define EN8 PB8
#define colorMap(val,r,g,b) strip.Color(max(0,map(val,0,1000,-(g)/2,(g))),max(0,map(val,0,1000,-(r)/2,(r))),max(0,map(val,0,1000,-(b)/2,(b))))

#define BIKE PA4
#define BIKE_LOW 1700
#define BIKE_HIGH 2200
#define CYMBAL PB10
#define CYMBAL_TIME 500
#define NOODLE PA1

#define PUMP PA0

#define ADV 60

WS2812B strip = WS2812B(NUM_LEDS);

int count = 0;
int enc_a = LOW;
int enc_b = LOW;
bool bike_on = false;
uint64 lastCymbalHit = 0;
bool cymbalOn = false;
bool noodleOn = false;

void setup() {
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);
  pinMode(EN4, OUTPUT);
  pinMode(EN5, OUTPUT);
  pinMode(EN6, OUTPUT);
  pinMode(EN7, OUTPUT);
  pinMode(EN8, OUTPUT);
  pinMode(BIKE, INPUT_ANALOG);
  pinMode(PUMP, INPUT_ANALOG);
  pinMode(NOODLE, INPUT_ANALOG);
  pinMode(CYMBAL, INPUT_PULLDOWN);
  digitalWrite(EN1, LOW);
  strip.begin();
  strip.show();

  Serial.begin(9600);
}

void readEnc() {
  int val = analogRead(BIKE);
  if (bike_on && val < BIKE_LOW) {
    bike_on = false;
    count += ADV;
  }
  if (!bike_on && val > BIKE_HIGH) {
    bike_on = true;
    count += ADV;
  }
  if (count >= 2000)
    count -= 2000;
  if (count < 0)
    count += 2000;

  uint64 time = millis();
  if (digitalRead(CYMBAL)) {
    if (!cymbalOn && lastCymbalHit-time > 1000) {
      lastCymbalHit = time;
      cymbalOn = true;
    }
  } else {
    cymbalOn = false;
  }

  uint16_t noodle = analogRead(NOODLE);
  noodleOn = noodle > 1000;
}

uint8_t mapper(uint16_t val, uint16_t a, uint16_t b, uint8_t c, uint8_t d) {
  uint64_t e = (uint64_t) d - (uint64_t) c;
  uint64_t f = (uint64_t) val - (uint64_t) a;
  uint64_t g = (uint64_t) b - (uint64_t) a;
  return (uint8_t) (c + e * f / g);
}

void showColor(int stripEnable, uint8 r, uint8 g, uint8 b) {
  long ctime = min(CYMBAL_TIME, millis() - lastCymbalHit);
  long cbrightness = map(ctime*ctime, 0, CYMBAL_TIME*CYMBAL_TIME, 255, 0);

  digitalWrite(stripEnable, HIGH);
  for(uint16_t i=0; i<strip.numPixels(); i++)
  {
    int val = 1000 - abs(3*(32*i+count)%2000 - 1000);
    int32_t color = colorMap(val, r, g, b);
    int32_t mixedColor = strip.Color(
      mapper(ctime, 0, CYMBAL_TIME, cbrightness, (color>>16)&0xff),
      mapper(ctime, 0, CYMBAL_TIME, cbrightness, (color>>8)&0xff),
      mapper(ctime, 0, CYMBAL_TIME, cbrightness, color&0xff)
    );
    if (noodleOn) {
      strip.setPixelColor(i, 0xff0000);
    } else {
      strip.setPixelColor(i, mixedColor);
    }
  }
  readEnc();
  strip.show();
  readEnc();
  delay(1);
  readEnc();
  delay(1);
  readEnc();
  delay(1);
  readEnc();
  digitalWrite(stripEnable, LOW);

}

void loop() {
  int16 pump = analogRead(PUMP);
  count = sqrt(pump)*20;

  showColor(EN1, 30,  0, 80);
  showColor(EN2,  0, 80, 40);
  showColor(EN4, 40, 100, 0);
  showColor(EN3, 80, 80,  0);
  showColor(EN5, 80, 40,  0);
  showColor(EN6, 80,  0, 20);
  showColor(EN7,  0,  60, 60);
  showColor(EN8,  60,  0, 60);
}
