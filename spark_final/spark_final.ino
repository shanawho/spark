#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

// include the SD library:
#include <SPI.h>
#include <SD.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define RECORD_BUT 13
#define MODE_BUT 11
#define NP_PIN 6
#define PELT 9

// include one (0th) for mode.
#define NUM_NEOPIXELS 19

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_NEOPIXELS, NP_PIN, NEO_GRB + NEO_KHZ800);

const int red[3] = {225, 20, 20};
const int blue[3] = {127, 203, 223};

int recordState = 0;         // variable for reading the pushbutton status
int lastRecordState = 0;
int modeState = 0;           // variable for reading the pushbutton status
int lastModeState = 0;
bool playback = false;       // start on recording mode.

//save last location
float lat, lon;


// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
File myFile;
String fileName = "datalog.csv";

// change this to match your SD CARD CS PIN
const int chipSelect = A5;

/*=========================================================================
    APPLICATION SETTINGS
    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

float threshold = .5;

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void) {
  pinMode(RECORD_BUT, INPUT);
  pinMode(MODE_BUT, INPUT);
  pinMode(PELT, OUTPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);


  // SD Card initialization
  Serial.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }


 
  // print the type of card
  Serial.print("\nCard type: ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }


  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);


  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
//  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);



  // Initialize chip
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");




  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);
  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  // un-comment for debugging
  //Serial.println(F("Bluefruit Connected"));
  //Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set Bluefruit to DATA mode
  ble.setMode(BLUEFRUIT_MODE_DATA);

  // debug LED, enabled/disabled by buttons 1 & 2
  pinMode(13, OUTPUT);

}

/**************************************************************************/
/*!
    @brief  Helper functions
*/
/**************************************************************************/

boolean near(float a, float b) {
  return (abs(a-b) < threshold);
}

// Save location onto card
void saveLocation(float lat, float lon) {
  // SD card code

   // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(fileName, FILE_WRITE);

  
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.println("Writing");
    Serial.print("Lat: "); Serial.print(lat, 4); // 4 digits of precision!
    Serial.print('\t');
    Serial.print("Lon: "); Serial.print(lon, 4); // 4 digits of precision!
    Serial.print('\t');

    myFile.print(lat, 4);
    myFile.print(",");
    myFile.print(lon, 4);
    myFile.print("\n");
    
  // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening "+fileName);
  }
  
}


// Checks if location is already saved on SD card
boolean alreadySaved(float lat, float lon) {
  // dummy filler for now
  // Check by looping through locations on SD card

  String currLat = "";
  String currLon = "";
  String temp = "";

  myFile = SD.open(fileName);
  if (myFile) {
    Serial.println("reading from " + fileName);

    while (myFile.available()) {

      char ch = myFile.read();

      if (ch == '\n') {
        // NEW LINE means we have a lat and lon
        currLon = temp;
        temp = "";

        Serial.println("Lat: " + currLat);
        Serial.println("Lon: " + currLon);

        // Check if this lat, lon i s already logged
        if (near(currLat.toFloat(), lat) && near(currLon.toFloat(), lon)) {
          myFile.close();
          return true;
        }
      } else if (ch == ',') {
        currLat = temp;
        temp = "";
      } else {
        temp = temp + ch;
      }

    }
    Serial.println("closing file");
    myFile.close();
    return false;
    
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening "+fileName);
    return false;
  }
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

void loop(void)
{
  recordState = digitalRead(RECORD_BUT);
  modeState = digitalRead(MODE_BUT);
  
  if (lastModeState != modeState) { // state has changed.
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
  }
  lastModeState = modeState;

  if (lastRecordState != recordState) { // state has changed.
    if (recordState == HIGH) {
      Serial.println("Record button pressed");
      // get GPS location from phone
      // store GPS on SD card
  
      if (!playback){
        Serial.println("I should be cooling down now");
        if (!alreadySaved(lat, lon)) {
          saveLocation(lat, lon);
        }
      }
      
    }
  }
  lastRecordState = recordState;
  delay(50);

  
  
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;

    // GPS Location
  if (packetbuffer[1] == 'L' && lat != parsefloat(packetbuffer+2) && lon != parsefloat(packetbuffer+10)) {
    lat = parsefloat(packetbuffer+2);
    lon = parsefloat(packetbuffer+6);
    Serial.print("Lat: "); Serial.print(lat, 4); // 4 digits of precision!
    Serial.print('\t');
    Serial.print("Lon: "); Serial.print(lon, 4); // 4 digits of precision!
    Serial.print('\t');

    // Simple if / else right now
    if (playback) {
      if (alreadySaved(lat, lon)) {
        Serial.println("already saved");
        Serial.println("I should be cooling down now");
        coolDown();
        fadeInOut(5);
        peltOff();
      }
      
    }
   
  }
}
