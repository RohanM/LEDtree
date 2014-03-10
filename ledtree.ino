#include <FastLED.h>
#include <Wire.h>

#define NUM_STRIPS 5
#define NUM_LEDS 50

#define SONAR_DEVICE 112


#define DATA_PIN_0 0
#define DATA_PIN_1 1
#define DATA_PIN_2 2
#define DATA_PIN_3 3
#define DATA_PIN_4 4

CRGB leds[NUM_STRIPS][NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2811, DATA_PIN_0, BRG>(leds[0], NUM_LEDS);
  FastLED.addLeds<WS2811, DATA_PIN_1, BRG>(leds[1], NUM_LEDS);
  FastLED.addLeds<WS2811, DATA_PIN_2, BRG>(leds[2], NUM_LEDS);
  FastLED.addLeds<WS2811, DATA_PIN_3, BRG>(leds[3], NUM_LEDS);
  FastLED.addLeds<WS2811, DATA_PIN_4, BRG>(leds[4], NUM_LEDS);
  FastLED.clear();

  Wire.begin();                // join i2c bus (address optional for master)
  Serial.begin(9600);          // start serial communication at 9600bps
}


void loop() {
  //illuminateAll();
  
  int level = readSonar();
  Serial.println("Level:");
  Serial.println(level);
  Serial.println("..");
  
  if(level > 0) {
    setLedLevel(level);
  }
  delay(250);
}


void setLedLevel(int level) {
  CRGB colours[] = {CRGB::Lime, CRGB::Pink, CRGB::Green, CRGB::Blue, CRGB::Purple};
  CRGB colour;

  for(int i=0; i < NUM_STRIPS; i++) {
    for(int j=0; j < NUM_LEDS; j++) {
      // j - 0 to 50
      if(j*20 < level) {
        colour = colours[i];
      } else {
        colour = CRGB::Black;
      }
      leds[i][j] = colour;
    }
  }
  
  FastLED.show();
}


int readSonar() {
  // step 1: instruct sensor to read echoes
  Wire.beginTransmission(SONAR_DEVICE); // transmit to device #112 (0x70)
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
  Wire.beginTransmission(SONAR_DEVICE); // transmit to device #112
  Wire.write(byte(0x02));      // sets register pointer to echo #1 register (0x02)
  Wire.endTransmission();      // stop transmitting

  // step 4: request reading from sensor
  Wire.requestFrom(SONAR_DEVICE, 2);    // request 2 bytes from slave device #112

  // step 5: receive reading from sensor
  int reading = 0;
  if(2 <= Wire.available()) {  // if two bytes were received
    int high = Wire.read();
    int low = Wire.read();
    reading = (high << 8) | low;
  
    //reading = Wire.read();  // receive high byte (overwrites previous reading)
    //reading = reading << 8;    // shift high byte to be high 8 bits
    //reading |= Wire.read(); // receive low byte as lower 8 bits

    Serial.println(high);
    Serial.println(low);
    Serial.println(reading);   // print the reading
    
    Serial.println("..");
    
  }
  
  return reading;
}
