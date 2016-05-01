/*********************************************************************
 * Critical Making Neck Pillow Team
 * Feather Bluefruit code to send proximity to present GPS coordinates
 * (also takes in controller button presses for demo purposes)
 * 
 This is built upon an example from Adafruit for their 
 nRF51822 based Bluefruit LE modules:
 Pick one up today in the adafruit shop!
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!
 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

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


//save last location
float lat, lon, alt;


// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
File myFile;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
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

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);
Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


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

// preset GPS coordinates for four points
// currently Berkeley, San Francisco Golden Gate Bridge, the Eiffel Tower, and the South Pole
//float gpsx[4] = {37.8759, 37.8197, 48.8582, -90.0000};
//float gpsy[4] = {-122.259, -122.4786, 2.2946, 0.0000};
//float threshold = 0.01; // x and y proximity for being "in the zone"
int gps_zone_old = 0, gps_zone_new = 0; // 0 or 1-4 when inside a zone

int fsrReading;
int threshold = 500;

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  while (!Serial);  // required for Flora & Micro
  delay(500);

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
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);


  
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

// Write to card

void writeCard(float lat, float lon) {

}



// Save location onto card
void saveLocation(float lat, float lon) {
  // SD card code

   // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("locations.txt", FILE_WRITE);

  
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to locations.txt...\t");
    Serial.print("Lat: "); Serial.print(lat, 4); // 4 digits of precision!
    Serial.print('\t');
    Serial.print("Lon: "); Serial.print(lon, 4); // 4 digits of precision!
    Serial.print('\t');

    myFile.println(lat, 4);
    myFile.print(", ");
    myFile.print(lon, 4);
    
  // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  
}

// Checks if location is already saved on SD card
boolean alreadySaved(float lat, float lon) {
  // dummy filler for now
  // Check by looping through locations on SD card
  return false;
}


// Alert user through make cold or buzz
boolean alertUser() {
  
}

void loop(void)
{


/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/


  
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;

  // Buttons
  if (packetbuffer[1] == 'B') {
    uint8_t buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';
    // print button number to Raspberry Pi (if 1-4)
    if (pressed && buttnum > 0 && buttnum < 5) {
      Serial.print(buttnum);
    } else if (pressed && buttnum > 4 && buttnum < 9) {
      // send 0 to simulate leaving zone for arrow buttons 5-8
      Serial.print(0);
    }

    // for debug LED
    if (buttnum == 1) {
      digitalWrite(13, HIGH); // reference LED    
    }
    if (buttnum == 2) {
      digitalWrite(13, LOW);    
    }
  }

    // GPS Location
  if (packetbuffer[1] == 'L' && lat != parsefloat(packetbuffer+2) && lon != parsefloat(packetbuffer+10)) {
    lat = parsefloat(packetbuffer+2);
    lon = parsefloat(packetbuffer+6);
    alt = parsefloat(packetbuffer+10);
//    Serial.print("GPS Location\t");
//    Serial.print("Lat: "); Serial.print(lat, 4); // 4 digits of precision!
//    Serial.print('\t');
//    Serial.print("Lon: "); Serial.print(lon, 4); // 4 digits of precision!
//    Serial.print('\t');
//    Serial.print(alt, 4); Serial.println(" meters");
  }


  // If the current lat, lon is already saved, 
  // then the scarf should alert (buzz/make cold)
  if (alreadySaved(lat, lon)) {
    alertUser();
  }

  // hook this up to button later

  if (!alreadySaved(lat, lon)) {
    saveLocation(lat, lon);
  }
 
  //delay(100);



  
}
