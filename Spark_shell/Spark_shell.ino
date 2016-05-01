#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define PIN2 9

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel modeLED = Adafruit_NeoPixel(60, PIN2, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const int recordButton = 11;     // the number of the pushbutton pin
const int modeButton = 10;     // the number of the pushbutton pin

int recordState = 0;         // variable for reading the pushbutton status
int modeState = 0;         // variable for reading the pushbutton status
bool ledMode = true;

void setup() {
  pinMode(recordButton, INPUT);
  pinMode(modeButton, INPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  modeLED.begin();
  modeLED.show();
}

void loop() {
  recordState = digitalRead(recordButton);
  modeState = digitalRead(modeButton);

  if (recordState == HIGH) {
    // get GPS location from phone
    // store GPS on SD card
    // vibrate

    // this logic is the "detects a marked area" logic
    // here for testing
    if (ledMode){
      colorWipe(strip.Color(255, 0, 0), 50); // Red
      delay(1000);
      colorWipe(strip.Color(0, 0, 0), 50); // Off
    }
  } 
  
  if (modeState == HIGH) {
    ledMode = !ledMode;

    if (ledMode == true){
      // turn on neopixel underneath mode button
      colorWipe2(modeLED.Color(255, 255, 255), 50); // white
    } else {
      colorWipe2(modeLED.Color(0, 0, 0), 50); // off
    }
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Fill the dots one after the other with a color
void colorWipe2(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    modeLED.setPixelColor(i, c);
    modeLED.show();
    delay(wait);
  }
}
