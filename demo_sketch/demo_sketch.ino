#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define RECORD_BUT 13
#define MODE_BUT 11
#define NP_PIN 6
#define PELT 9

#define NUM_NEOPIXELS 19
// include one (0th) for mode.

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_NEOPIXELS, NP_PIN, NEO_GRB + NEO_KHZ800);

const int red[3] = {225, 20, 20};
const int blue[3] = {127, 203, 223};

int recordState = 0;         // variable for reading the pushbutton status
int modeState = 0;           // variable for reading the pushbutton status
bool playback = false;       // start on recording mode.

void setup() {
  pinMode(RECORD_BUT, INPUT);
  pinMode(MODE_BUT, INPUT);
  pinMode(PELT, OUTPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  recordState = digitalRead(RECORD_BUT);
  modeState = digitalRead(MODE_BUT);
  
  if (modeState == HIGH) {
    Serial.println("Mode button pressed");
    playback = !playback;
    Serial.println("Playback is now ");
    Serial.println(playback);
    if (playback){
      strip.setPixelColor(0, strip.Color(blue[0], blue[1], blue[2]));
    } else {
      strip.setPixelColor(0, strip.Color(0, 0, 0));
    }
    strip.show();
  }



  if (recordState == HIGH) {
    Serial.println("Record button pressed");
    // get GPS location from phone
    // store GPS on SD card

    if (playback){
      Serial.println("I should be cooling down now");
        coolDown();
        fadeInOut(5);
        peltOff();
    }
  }
  delay(200);
}

//Fade red in & out n times.
void fadeInOut(uint8_t n) {
  for(int i = 0; i < n; i++) {
    for(double ratio = 0; ratio < 1; ratio += 0.02) {
      for(uint16_t i=1; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(red[0] * ratio, red[1] * ratio, red[2] * ratio));
      }
      strip.show();
      delay(10);
    }
    for(double ratio = 1; ratio > 0; ratio -= 0.02) {
      for(uint16_t i=1; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(red[0] * ratio, red[1] * ratio, red[2] * ratio));
      }
      strip.show();
      delay(10);
    }
  }
}

void coolDown() {
  analogWrite(PELT, 255);
}

void peltOff() {
  analogWrite(PELT, 0);
}
