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

#define topFM  107900000            // Top of the FM Dial Range in USA
#define botFM   87500000            // Bottom of the FM Dial Range in USA
#define incrFM    200000            // FM Channel Increment in USA
// define incrFM   100000           // FM Channel Increment - certain countries.
// define incrFM    50000           // FM Channel Increment - certain countries...
#define encoderPinA  2
#define encoderPinB  3
#define buttonPin 13


int encoderPos = 0;
int buttonState;
int lastButtonState = LOW;
long lastDebounceTime= 0;
long debounceDelay = 50;


byte ls247pins[] = {
  4, 5, 6, 7};
byte anodepins[] = {
  9, 10, 11, 12};
byte dp = 8; // decimal point
byte number[] = {
  15, 8, 8, 7};

int serialCount = 0;
int serialArray[2];

long frequency = 99300000;          // the default initial frequency in Hz
long newFrequency = 0;
boolean gOnAir = true;             // Initially, NOT On The Air...

void setup() {
  Serial.begin(9600);                 //for debugging
  initRadio();
}

void initRadio() {
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  for (int i=0; i<4; i++) {
    pinMode(ls247pins[i],OUTPUT);
    pinMode(anodepins[i],OUTPUT);
  }
  pinMode(dp, OUTPUT);  
  digitalWrite(dp, HIGH);
  attachInterrupt(0, doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2
  Wire.begin();                       // join i2c bus as master
  transmitter_standby(frequency);
  delay(2000);
  transmitter_setup(frequency);
}

void loop() {
  readEncoder();
  // Serial.println( frequency, DEC );
  print_lcd_frequency(frequency);
  for (int i=0; i<4; i++) {
    led7segWriteDigit(i, number[i]);
  }
  check_serial();
}

void led7segWriteDigit(int digit, int value) {
  if (digit < 4) {
    if (value < 16) {
      // blank all       
      for (int i=0; i<4; i++) {
        digitalWrite(anodepins[i],LOW);
      }

      if (digit==3) (digitalWrite(dp,LOW));
      else (digitalWrite(dp,HIGH));

      for (int i=0; i<4; i++) {
        digitalWrite(ls247pins[i],(bitRead(value,i)));
      } 
    }
    // turn on the appropriate digit
    digitalWrite(anodepins[digit],HIGH);
  }
  delay(1);
}


void readEncoder() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    buttonState = reading;
  }
  if(buttonState == LOW) {

    Serial.println("Pressed");
    if ( gOnAir ) {
      transmitter_standby( frequency );
      buttonState = HIGH;
    }
    else {
      set_freq( frequency );
      Serial.println(frequency);
      delay(1000);
      buttonState = HIGH;
    }

  }
  lastButtonState = reading;
}

void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   */
  if (digitalRead(encoderPinA) == digitalRead(encoderPinB)) {
    frequency -= incrFM; 
    delay(200);
    frequency = constrain( frequency, botFM, topFM);  // Keeps us in range...
  } 
  else {
    frequency += incrFM; 
    delay(200);
    frequency = constrain( frequency, botFM, topFM);  // Keeps us in range...
  }
}

void print_lcd_frequency(long input) {
   int freq;
   
   freq = (int) (input / 100000);
   number[0] = freq / 1000; // should be 0 or 1
   if (number[0] == 0) number[0] = 15; // display blank LCD character
   freq = freq % 1000;
   number[1] = freq / 100;
   freq = freq % 100;
   number[2] = freq / 10;
   freq = freq % 10;
   number[3] = freq;
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

void transmitter_standby(long aFrequency) {
  //i2c_send(0x00, B10100000); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power OFF
  i2c_send(0x00, B00100000); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power OFF

  delay(100);
  gOnAir = false;
}

void set_freq(long aFrequency) {
  int new_frequency;

  // New Range Checking... Implement the (experimentally determined) VFO bands:
  if (aFrequency < 88500000) {                       // Band 3
    i2c_send(0x08, B00011011);
    // Serial.println("Band 3");
  }  
  else if (aFrequency < 97900000) {                 // Band 2
    i2c_send(0x08, B00011010);
    // Serial.println("Band 2");
  }
  else if (aFrequency < 103000000) {                  // Band 1 
    i2c_send(0x08, B00011001);
    // Serial.println("Band 1");
  }
  else {
    // Must be OVER 103.000.000,                    // Band 0
    i2c_send(0x08, B00011000);
    // Serial.println("Band 0");
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

void check_serial() {
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    Serial.write(inByte); // communicate received message back to Max for debugging
    // set random frequency
    /*
    if (inByte == 255) {
     transmitter_standby(frequency);
     
     frequency = tempFrequency;
     set_freq(frequency);
     }
     */
    if (inByte == 255) serialCount = 0;
    // set incoming byte into a temporary array and move through it
    // these values will be reassigned
    serialArray[serialCount] = inByte;
    Serial.write(serialArray[0]);
    Serial.write(serialArray[1]);
    serialCount++;
    if (serialCount == 2) {
      frequency = (serialArray[0] * 100) + serialArray[1];
      // reset the serial count to receive the next message
      serialCount = 0;
    }
    // set transmitter into standby mode
    if (inByte == 254) {
      transmitter_standby(frequency);
    }
    
  }
}






