//
//
//  ARRRduino-FM: Part 15 FM Broadcasting for the Arduino
//
//  By:   Mike Yancey
//  Date: October 7, 2008
//  Version 3.0
//  Language: Arduino 0013
//  Developed Around: October 7-12, 2008
//  Updated to Version 3.0: February 2009
//  
//  Description: Uses an Arduino or compatible to control an 
//               NS73M FM Transmitter module (available from 
//               Sparkfun) on the I2C bus.
//
//  Goals for Version 2:
//    Upgrade for Arduino 0012, which includes an LCD Library
//    Move to a single Rotary Encoder control instead of buttons.
//    Encoder will have a bump-switch to use for a On/Off-Air button.
//    Moving to an encoder will simplify packaging
//
//  Experimentally derived Band Settings for VFO ...
//		Band 0: 83.78 - 91.72
//		Band 1: 88.74 - 98.28
//		Band 2: 93.10 - 104.00
//		Band 3: 99.50 - 112.72
//      ref: http://cba.sakura.ne.jp/sub04/jisaku36.htm (translated)

#include <Wire.h>

#define setButton 4                 // set button on pin 4

#define topFM  107900000            // Top of the FM Dial Range in USA
#define botFM   87500000            // Bottom of the FM Dial Range in USA
#define incrFM    200000            // FM Channel Increment in USA
// define incrFM   100000           // FM Channel Increment - certain countries.
// define incrFM    50000           // FM Channel Increment - certain countries...


long frequency = 90300000;          // the default initial frequency in Hz
long newFrequency = 0;
boolean gOnAir = true;             // Initially, NOT On The Air...

#define upButton 6                  // up button on pin 6
#define downButton 5                // down button on pin 5
#define setButton 4                 // set button on pin 4


void setup() {
  Serial.begin(9600);                 //for debugging
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(setButton, INPUT_PULLUP);
  Wire.begin();                       // join i2c bus as master
  transmitter_setup( frequency );
  randomSeed(analogRead(3) + analogRead(5));
}



void loop() {
  // transmitter_standby(frequency);

  /*
  if (digitalRead(upButton) == LOW) {
   
   
   frequency += incrFM;         // 200kHz steps for North American channel spacing
   
   delay(100);
   frequency = constrain( frequency, botFM, topFM);  // Keeps us in range...
   
   // Serial.println( frequency, DEC );
   //   transmitter_standby( frequency );
   }
   
   if (digitalRead(downButton) == LOW) {
   frequency -= incrFM;                              // 200kHz steps for North American channel spacing
   delay(100);
   frequency = constrain( frequency, botFM, topFM);  // Keeps us in range...
   
   // Serial.println( frequency, DEC );
   //   transmitter_standby( frequency );
   }
   
   if (digitalRead(setButton) == LOW) {
   // Create a 'toggle' - pressing set while OnAir - set's StandBy and
   // if we're already StandBy, set the Frequency and go OnAir... 
   if ( gOnAir ) {
   transmitter_standby( frequency );
   }
   else {
   set_freq( frequency );
   //     saveFrequency( frequency );     // Save the Frequency to EEPROM Memory
   delay(1000);
   }
   }
   */
  check_serial();
  // Serial.println(newFrequency);
}



void transmitter_setup( long initFrequency )
{
  i2c_send(0x0E, B00000101); //Software reset

  i2c_send(0x01, B10110100); //Register 1: forced subcarrier, pilot tone on

  i2c_send(0x02, B00000011); //Register 2: Unlock detect off, 2mW Tx Power

  set_freq( initFrequency);

  //i2c_send(0x00, B10100001); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power on
  i2c_send(0x00, B00100001); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power on

  i2c_send(0x0E, B00000101); //Software reset

  i2c_send(0x06, B00011110); //Register 6: charge pumps at 320uA and 80 uA
}

void transmitter_standby( long aFrequency )
{
  //i2c_send(0x00, B10100000); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power OFF
  i2c_send(0x00, B00100000); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power OFF


  delay(100);
  gOnAir = false;
}

void set_freq( long aFrequency )
{
  int new_frequency;

  // New Range Checking... Implement the (experimentally determined) VFO bands:
  if (aFrequency < 88500000) {                       // Band 3
    i2c_send(0x08, B00011011);
    Serial.println("Band 3");
  }  
  else if (aFrequency < 97900000) {                 // Band 2
    i2c_send(0x08, B00011010);
    Serial.println("Band 2");
  }
  else if (aFrequency < 103000000) {                  // Band 1 
    i2c_send(0x08, B00011001);
    Serial.println("Band 1");
  }
  else {
    // Must be OVER 103.000.000,                    // Band 0
    i2c_send(0x08, B00011000);
    Serial.println("Band 0");
  }


  new_frequency = (aFrequency + 304000) / 8192;
  byte reg3 = new_frequency & 255;                  //extract low byte of frequency register
  byte reg4 = new_frequency >> 8;                   //extract high byte of frequency register
  i2c_send(0x03, reg3);                             //send low byte
  i2c_send(0x04, reg4);                             //send high byte

  // Retain old 'band set' code for reference....  
  // if (new_frequency <= 93100000) { i2c_send(0x08, B00011011); }
  // if (new_frequency <= 96900000) { i2c_send(0x08, ); }
  // if (new_frequency <= 99100000) { i2c_send(0x08, B00011001); }
  // if (new_frequency >  99100000) { i2c_send(0x08, B00011000); }

  i2c_send(0x0E, B00000101);                        //software reset  

  Serial.print("Frequency changed to ");
  Serial.println(aFrequency, DEC);

  //i2c_send(0x00, B10100001); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power ON
  i2c_send(0x00, B00100001); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power ON


  gOnAir = true;
}

void i2c_send(byte reg, byte data)
{ 
  Wire.beginTransmission(B1100111);               // transmit to device 1100111
  Wire.write(reg);                                 // sends register address
  Wire.write(data);                                // sends register data
  Wire.endTransmission();                         // stop transmitting
  delay(5);                                       // allow register to set
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
/*
void serialEvent() {
 while (Serial.available()) {
 if (Serial.read() == 255) {
 randomSeed(analogRead(3) + analogRead(5));
 long tempFrequency = (long) random(875, 1079);
 newFrequency = tempFrequency * 100000;
 digitalWrite(13, LOW);
 delay(100);
 digitalWrite(13, HIGH);
 delay(100);
 }
/*
 // get the new byte:
 char inChar = (char)Serial.read(); 
 // add it to the inputString:
 inputString += inChar;
 // if the incoming character is a newline, set a flag
 // so the main loop can do something about it:
 if (inChar == '\n') {
 stringComplete = true;
 } 
 */

void check_serial() {
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    Serial.write(inByte);
    // press enter
    if (inByte == 255) {
      transmitter_standby(frequency);
      long randomFrequency = (long) random(875, 1079);
      Serial.write(randomFrequency/10);
      long tempFrequency = randomFrequency * 100000;
      frequency = tempFrequency;
      if (gOnAir) { 
        transmitter_standby( frequency );
      }
      else {
        set_freq(frequency);
        //     saveFrequency( frequency );     // Save the Frequency to EEPROM Memory
      }
      delay(500);
    }
    // press tab
    if (inByte == 254) {
      transmitter_standby(frequency);
    }
  }
}



