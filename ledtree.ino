#include <FastLED.h>
#include <Wire.h>

#define NUM_STRIPS 5
#define NUM_LEDS 50

#define NUM_SONAR_SAMPLES 10

#define LED_DATA_PIN_0 2
#define LED_DATA_PIN_1 3
#define LED_DATA_PIN_2 4
#define LED_DATA_PIN_3 5
#define LED_DATA_PIN_4 6

int sonarDevices[] = {112, 113, 114, 115, 116};

int counter;
CRGB leds[NUM_STRIPS][NUM_LEDS];
int nextSonar = 0;
int sonarReadings[NUM_STRIPS][NUM_SONAR_SAMPLES];
int sonarIndices[NUM_STRIPS];
int proximities[NUM_STRIPS];




/* TODO

Effect for when all sonars have someone near them (value < 75 or so). Ideas:
- Put saturated rainbow colours in bottom value while conditions last
  - In a spiral. Use offset to make each strip a little different, but in sequence.
- Draw a spiral around the tree, starting at the base
- 


Rainbow rain
- Orange glow reaches top on all sides
- Rainbow colours rain down
  - Twinkling effect - keep hue the same, random frames it sparkles bright white
- Can only be triggered once every 10 m or so

*/


void setup() {
  initLEDs();
  initSonars();

  Serial.begin(9600);
  counter = 0;
}

void initLEDs() {
  FastLED.addLeds<WS2811, LED_DATA_PIN_0, BRG>(leds[0], NUM_LEDS);
  FastLED.addLeds<WS2811, LED_DATA_PIN_1, BRG>(leds[1], NUM_LEDS);
  FastLED.addLeds<WS2811, LED_DATA_PIN_2, BRG>(leds[2], NUM_LEDS);
  FastLED.addLeds<WS2811, LED_DATA_PIN_3, BRG>(leds[3], NUM_LEDS);
  FastLED.addLeds<WS2811, LED_DATA_PIN_4, BRG>(leds[4], NUM_LEDS);
  FastLED.clear();
}

void initSonars() {
  Wire.begin();

  // Zero data
  for(int i=0; i < NUM_STRIPS; i++) {
    sonarIndices[i] = 0;
    proximities[i] = 0;
    for(int j=0; j < NUM_SONAR_SAMPLES; j++) {
      sonarReadings[i][j] = 0;
    }
  }
}


void loop() {
  readNextSonar();

  for(int i=0; i < NUM_STRIPS; i++) {
    moveAndIntensify(i);
    setBottomValue(i);
  }
  FastLED.show();

  counter++;
}


void moveAndIntensify(int stripNo) {
  for(int i=NUM_LEDS-1; i > 0; i--) {
    leds[stripNo][i] = leds[stripNo][i-1];

    if(i > NUM_LEDS/4*3) {
      leds[stripNo][i] = intensify(leds[stripNo][i]);
    }
  }
}

CRGB intensify(CRGB colour) {
  colour.red   *= 1.5;
  colour.green *= 1.2;
  colour.blue  *= 1.5;

  return colour;
}

void setBottomValue(int stripNo) {
  float sonarEffect = calcSonarEffect(proximities[stripNo]);

  // hue:        90 (green) to 30 (orange)
  // saturation: 180 (most) to 255 (full)
  // value:      70 (BPB) to 255 (full)

  int offset, h, s, v;
  offset = stripNo * 10;
  h = 90 - (60 * sonarEffect);
  s = 180 + (75 * sonarEffect);
  v = backgroundPulseBrightness(counter, offset) * (1 + (sonarEffect * 2.6));

  //Serial.println(sonarEffect);
  
  leds[stripNo][0] = CHSV(h, s, v);
}

float calcSonarEffect(int sonarValue) {
  float sonarEffect = 0;

  if(sonarValue < 255) {
    sonarEffect = (255 - sonarValue) / 255.0;
  }

  return sonarEffect;
}

int backgroundPulseBrightness(int index, int offset) {
  return (sin((index+offset) / 4.0) + 1.0) * 30.0 + 10;
}

int sonarBrightness(int sonar) {
  return (255 - sonar) / 2;
}

void readNextSonar() {
  takeSonarReading(nextSonar);
  proximities[nextSonar] = avgSonarReading(nextSonar);

  nextSonar++;
  if(nextSonar >= NUM_STRIPS) {
    nextSonar = 0;
  }
}


void takeSonarReading(int sonar) {
  int reading = readSonar(sonarDevices[sonar]);

  // Record zeros as distant, not close
  if(reading == 0) {
    // 700 cm is furthest common reading
    reading = 700;
  }

  sonarReadings[sonar][sonarIndices[sonar]] = reading;
  incrementSonarIndex(sonar);
}

void incrementSonarIndex(int sonar) {
  sonarIndices[sonar]++;
  if(sonarIndices[sonar] >= NUM_SONAR_SAMPLES) {
    sonarIndices[sonar] = 0;
  }
}

int avgSonarReading(int sonar) {
  int sum = 0;

  for(int i=0; i < NUM_SONAR_SAMPLES; i++) {
    sum += sonarReadings[sonar][i];
  }

  return sum / NUM_SONAR_SAMPLES;
}




// This function is slow, taking about 150 ms for a reading
// About half of this is the 70 ms delay waiting for the reading to come back from the sonar
int readSonar(int device) {
  // step 1: instruct sensor to read echoes
  Wire.beginTransmission(device); // transmit to device #112 (0x70)
                               // the address specified in the datasheet is 224 (0xE0)
                               // but i2c adressing uses the high 7 bits so it's 112
  Wire.write(byte(0x00));      // sets register pointer to the command register (0x00)  
  Wire.write(byte(0x51));      // command sensor to measure in "inches" (0x50) 
                               // use 0x51 for centimeters
                               // use 0x52 for ping microseconds
  Wire.endTransmission();      // stop transmitting

  // step 2: wait for readings to happen
  delay(70);                   // datasheet suggests at least 65 milliseconds

  // step 3: instruct sensor to return a particular echo reading
  Wire.beginTransmission(device); // transmit to device #112
  Wire.write(byte(0x02));      // sets register pointer to echo #1 register (0x02)
  Wire.endTransmission();      // stop transmitting

  // step 4: request reading from sensor
  Wire.requestFrom(device, 2);    // request 2 bytes from slave device #112

  // step 5: receive reading from sensor
  int reading = 0;
  if(2 <= Wire.available()) {  // if two bytes were received
    int high = Wire.read();
    int low = Wire.read();
    reading = (high << 8) | low;
  
    //reading = Wire.read();  // receive high byte (overwrites previous reading)
    //reading = reading << 8;    // shift high byte to be high 8 bits
    //reading |= Wire.read(); // receive low byte as lower 8 bits

    //Serial.println(high);
    //Serial.println(low);
    //Serial.println(reading);   // print the reading
    
    //Serial.println("..");
  }
  
  return reading;
}

