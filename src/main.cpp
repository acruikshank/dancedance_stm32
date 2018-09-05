#include <Arduino.h>
#include <WS2812B.h>

#define NUM_LEDS 50
#define NUM_STRIPS 50
#define EN1 PB9
#define EN2 PB4
#define EN3 PA2
#define EN4 PA3
#define EN5 PB7
#define EN6 PB6
#define EN7 PB5
#define EN8 PB8
#define colorMap(val,r,g,b) strip.Color(max(0,map(val,0,1000,-(g)/2,(g))),max(0,map(val,0,1000,-(r)/2,(r))),max(0,map(val,0,1000,-(b)/2,(b))))
#define asRed(r) ((r)<<16)
#define asGreen(g) ((g)<<8)
#define asBlue(b) (b)
#define fromRed(r) (((r)>>16)&0xff)
#define fromGreen(g) (((g)>>8)&0xff)
#define fromBlue(b) ((b)&0xff)
#define mixColor(x,c1,c2) strip.Color(map(x,0,1000,fromRed(c1),fromRed(c2)), map(x,0,1000,fromGreen(c1),fromGreen(c2)), map(x,0,1000,fromBlue(c1),fromBlue(c2)))

#define BIKE PA4
#define BIKE_LOW 1700
#define BIKE_HIGH 2200
#define CYMBAL PB10
// #define CYMBAL_TIME 500
#define NOODLE PA1

// states
#define INIT 0
#define ACTIVE 1

// commands
#define NO_COMMAND -1
#define INIT_COMMAND 0

#define NO_PARAMETER -1
#define OFFSET_PARAMETER 0

#define CYCLE_COMMAND 1
#define COUNT_PARAMETER 0

#define DRUM_COMMAND 2
#define DRUM_PARAMETER 0

#define CYMBOLS_COMMAND 3

#define ADV 1
#define DRUM_OFFSET 1800.0
#define DRUM_SLOPE 18.0
#define DRUM_ACC .0015

#define CYMBOL_ATTACK 200
#define CYMBOL_DECAY 2000
#define CYMBOL_FREQ .0002
#define R_PHASE -.012
#define G_PHASE -.028
#define B_PHASE -.025
#define PUMP PA0


WS2812B strip = WS2812B(NUM_LEDS);

float distances[NUM_LEDS*NUM_STRIPS];

long lastDrumHit[] = {0,0,0,0,0};
long lastCymbolHit = 0;
int32_t drumColor[] = {0xfe00000, 0xf9fe000, 0x00fe030, 0x0003fe0, 0x9d00fe0};
int count = 0;
int state = INIT;

// commands
int command = NO_COMMAND;
int parameter = NO_PARAMETER;

// parameters
int offset = 0;

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

  for (int i=0; i< NUM_STRIPS; i++) {
    for (int j=0; j< NUM_LEDS; j++) {
      float dx = 3*(NUM_STRIPS*.5 - i);
      float dy = (NUM_LEDS*.5 - j);
      distances[i*NUM_LEDS+j] = sqrt(dx*dx + dy*dy);
    }
  }

  Serial.begin(9600);
}

void resetCommand() {
  parameter = NO_PARAMETER;
  command = NO_COMMAND;
}

void readData() {
  if (Serial.available()) {
    int value = Serial.read();
    switch (command) {
      case NO_COMMAND:
        command = value;
        switch (command) {
          case CYMBOLS_COMMAND:
            lastCymbolHit = millis();
            resetCommand(); break;
            break;
          default:
            parameter = 0;
        }
        break;
      case INIT_COMMAND:
        switch (parameter) {
          case OFFSET_PARAMETER:
            offset = value;
            state = ACTIVE;
          default:
            resetCommand(); break;
        }
        break;
      case CYCLE_COMMAND:
        switch (parameter) {
          case COUNT_PARAMETER:
            count = value * ADV;
          default:
            resetCommand(); break;
        }
        break;
      case DRUM_COMMAND:
        switch (parameter) {
          case DRUM_PARAMETER:
            lastDrumHit[value] = millis();
          default:
            resetCommand(); break;
        }
        break;
      default:
        resetCommand(); break;
    }
  }
}

// void readEnc() {
  // int val = analogRead(BIKE);
  // if (bike_on && val < BIKE_LOW) {
  //   bike_on = false;
  //   count += ADV;
  // }
  // if (!bike_on && val > BIKE_HIGH) {
  //   bike_on = true;
  //   count += ADV;
  // }
  // if (count >= 2000)
  //   count -= 2000;
  // if (count < 0)
  //   count += 2000;
  //
  // uint64 time = millis();
  // if (digitalRead(CYMBAL)) {
  //   if (!cymbalOn && lastCymbalHit-time > 1000) {
  //     lastCymbalHit = time;
  //     cymbalOn = true;
  //   }
  // } else {
  //   cymbalOn = false;
  // }
  //
  // uint16_t noodle = analogRead(NOODLE);
  // noodleOn = noodle > 1000;
// }

long attackDecay(long time, long attack, long decay) {
  if (time < attack)
    return map(time, 0, attack, 0, 1000);
  if (time < attack+decay)
    return map(time-attack, 0, decay, 1000, 0.0);
  return 0;
}

void showColor(int stripEnable, uint8 r, uint8 g, uint8 b, int column) {
  // long ctime = min(CYMBAL_TIME, millis() - lastCymbolHit);
  // long cbrightness = map(ctime*ctime, 0, CYMBAL_TIME*CYMBAL_TIME, 255, 0);

  int drum = (int) (column / 10);
  float drumTime = lastDrumHit[drum] - millis();
  float drumDrop = DRUM_ACC * drumTime * drumTime;
  long cymbolTime = millis()-lastCymbolHit;
  int env = attackDecay(cymbolTime, CYMBOL_ATTACK, CYMBOL_DECAY);
  float cfreq = CYMBOL_FREQ * attackDecay(cymbolTime, 0, 2*CYMBOL_ATTACK+CYMBOL_DECAY) / 1000.0;

  digitalWrite(stripEnable, HIGH);
  for(uint16_t i=0; i<strip.numPixels(); i++)
  {
    // int pedalVal = 1000 - abs(40*((2*i+count)%50) - 1000);
    // int32_t color = colorMap(pedalVal, r, g, b);
    uint32_t color = (i+count) % 20 < 5 ? 0x2828282 : 0x000000;

    if (drumDrop > 50 - i) {
      int drumVal = constrain(DRUM_OFFSET - DRUM_SLOPE*(drumDrop - 50 + i), 0, 1000);
      color = mixColor(drumVal, color, drumColor[drum]);
    }

    // cymbol / stompbox
    // Atack decay curve base on hit time
    // 3 concentric sin waves, r, g and b
    // Same freq, phase is a function of time * c, where c varies slightly for each
    // for each wave, create a high colors from existing color, c: c|0xFF0000, c|0x00FF00, c|0x0000FF
    // also create low colors: c&0x00FFFF, c&0xFF00FF, c&0xFFFF00
    // if val=sin(freq*dist + cPhase*time) > 0, mixColor(val, c, highColor) otherwise mixColor(-val, c, lowColor)
    // combine colors something like this:
    if (env > 0) {
      // this is not actually mixing to zero
      float dist = distances[column*NUM_LEDS + i];
      int rval = (int) (env*sin((cfreq*dist + R_PHASE)*cymbolTime));
      int gval = (int) (env*sin((cfreq*dist + G_PHASE)*cymbolTime));
      int bval = (int) (env*sin((cfreq*dist + B_PHASE)*cymbolTime));
      uint32_t r = rval > 0 ? asRed(map(rval,0,1000,fromRed(color), 0xFF)) : asRed(map(-rval,0,1000,fromRed(color), 0x00));
      uint32_t g = gval > 0 ? asGreen(map(gval,0,1000,fromGreen(color), 0xFF)) : asGreen(map(-gval,0,1000,fromGreen(color), 0x00));
      uint32_t b = bval > 0 ? asBlue(map(bval,0,1000,fromBlue(color), 0xFF)) : asBlue(map(-bval,0,1000,fromBlue(color), 0x00));
      color = r | g | b;
    }

    strip.setPixelColor(i, color);
    // int32_t mixedColor = strip.Color(
    //   map(ctime, 0, CYMBAL_TIME, cbrightness, (color>>16)&0xff),
    //   map(ctime, 0, CYMBAL_TIME, cbrightness, (color>>8)&0xff),
    //   map(ctime, 0, CYMBAL_TIME, cbrightness, color&0xff)
    // );
    // if (noodleOn) {
    //   strip.setPixelColor(i, 0xff0000);
    // } else {
    //   strip.setPixelColor(i, mixedColor);
    // }
  }
  readData();
  strip.show();
  digitalWrite(stripEnable, LOW);
  readData();
  delay(1);
  readData();
  delay(1);
  readData();
  delay(1);
  readData();
}

void runInteractive() {
  int16 pump = analogRead(PUMP);
  // count = sqrt(pump)*20;

  // showColor(EN1, 30,  0, 80, 0+offset);
  // showColor(EN2,  0, 80, 40, 1+offset);
  // showColor(EN4, 40, 100, 0, 2+offset);
  // showColor(EN3, 80, 80,  0, 3+offset);
  // showColor(EN5, 80, 40,  0, 4+offset);
  // showColor(EN6, 80,  0, 20, 5+offset);
  // showColor(EN7,  0, 60, 60, 6+offset);
  // showColor(EN8, 60,  0, 60, 7+offset);

  showColor(EN1, 40, 40, 40, 0+offset);
  showColor(EN2, 40, 40, 40, 1+offset);
  showColor(EN4, 40, 40, 40, 2+offset);
  showColor(EN3, 40, 40, 40, 3+offset);
  showColor(EN5, 40, 40, 40, 4+offset);
  showColor(EN6, 40, 40, 40, 5+offset);
  showColor(EN7, 40, 40, 40, 6+offset);
  showColor(EN8, 40, 40, 40, 7+offset);
}


void loop() {
  if (state==INIT) {
    readData();
  } else if (state==ACTIVE) {
    runInteractive();
  }
}
