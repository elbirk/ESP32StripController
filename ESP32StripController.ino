/********************************************************************************************************
 ** ESP32Strip Controller
 ** ----------------------
 *********************************************************************************************************/  
 /*  
    License:
    --------   
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE. 
*/

#include <elapsedMillis.h>
#include <FastLED.h>
#define SERIAL_BUFFER_SIZE 4096

//Definiton of Major and Minor part of the firmware version. This value can be received using the V command.
//If something is changed in the code the number should be increased.
#define FirmwareVersionMajor 1
#define FirmwareVersionMinor 3

//Defines the max number of leds which is allowed per ledstrip.
#define MaxLedsPerStrip 448
#define configuredStrips 6
word configuredStripLength = 448;

CRGB leds[configuredStrips*MaxLedsPerStrip];



//Defines the Pinnumber to which the built in led is connected.
#define LedPin 2

//Variable used to control the blinking and flickering of the led
elapsedMillis BlinkTimer;
int BlinkMode;
elapsedMillis BlinkModeTimeoutTimer;


//Setup of the system. Is called once on startup.
void setup() {
  Serial.begin(921600);

  //Initialize the lib for the leds
  SetFastLedStripes(configuredStripLength);
  
  //Initialize the led pin
  pinMode(LedPin, OUTPUT);
  SetBlinkMode(0);
}

//Main loop of the programm gets called again and again.
void loop() {
  // put your main code here, to run repeatedly:

  //Check if data is available
  if (Serial.available()) {

    byte receivedByte = Serial.read();
    switch (receivedByte) {
      case 'L':
        //Set length of strips
        SetLedStripLength();
        break;
      case 'F':
        //Fill strip area with color
        Fill();
        break;
      case 'R':
        //receive data for strips
        ReceiveData();
        break;
      case 'O':
        //output data on strip
        OutputData();
        break;
      case 'C':
        //Clears all previously received led data
        ClearAllLedData();
        break;
      case 'V':
        //Send the firmware version
        SendVersion();
        break;
      case 'M':
        //Get max number of leds per strip
        SendMaxNumberOfLeds();
        break;
      case 'T':
        fill_solid( leds, MaxLedsPerStrip*configuredStrips, CRGB(255,0,0));
        FastLED.show();
        FastLED.delay(1000);
  //      FastLED.showColor(CRGB(0, 255, 0));
        fill_solid( leds, MaxLedsPerStrip*configuredStrips, CRGB(0,255,0));
        FastLED.show();
        FastLED.delay(1000);
 //       FastLED.showColor(CRGB(0, 0, 255));
        fill_solid( leds, MaxLedsPerStrip*configuredStrips, CRGB(0,0,255));
        FastLED.show();
        FastLED.delay(1000);
        ClearAllLedData();
        FastLED.show();
        break;
      default:
        // no unknown commands allowed. Send NACK (N)
        Nack();
        break;
    }


    SetBlinkMode(1);


  }
  Blink();
}


//Sets the mode for the blinking of the led
void SetBlinkMode(int Mode) {
  BlinkMode = Mode;
  BlinkModeTimeoutTimer = 0;
}

//Controls the blinking of the led
void Blink() {
  switch (BlinkMode) {
    case 0:
      //Blinkmode 0 is only active after the start of the Teensy until the first command is received.
      if (BlinkTimer < 1500) {
        digitalWrite(LedPin, 0);
      } else if (BlinkTimer < 1600) {
        digitalWrite(LedPin, 1);
      } else {
        BlinkTimer = 0;
        digitalWrite(LedPin, 0);
      }
      break;
    case 1:
      //Blinkmode 1 is activated when the Teensy receives a command
      //Mode expires 500ms after the last command has been received resp. mode has been set
      if (BlinkTimer > 30) {
        BlinkTimer = 0;
        digitalWrite(LedPin, !digitalRead(LedPin));
      }
      if (BlinkModeTimeoutTimer > 500) {
        SetBlinkMode(2);
      }
      break;
    case 2:
      //Blinkmode 2 is active while the Teensy is waiting for more commands
      if (BlinkTimer < 1500) {
        digitalWrite(LedPin, 0);
      } else if (BlinkTimer < 1600) {
        digitalWrite(LedPin, 1);
      } else if (BlinkTimer < 1700) {
        digitalWrite(LedPin, 0);
      } else if (BlinkTimer < 1800) {
        digitalWrite(LedPin, 1);
      } else {
        BlinkTimer = 0;
        digitalWrite(LedPin, 0);
      }
    default:
      //This should never be active
      //The code is only here to make it easier to determine if a wrong Blinkcode has been set
      if (BlinkTimer > 2000) {
        BlinkTimer = 0;
        digitalWrite(LedPin, !digitalRead(LedPin));
      }
      break;
  }

}


//Outputs the data in the ram to the ledstrips
void OutputData() {
  FastLED.show();
  Ack();
}


//Fills the given area of a ledstrip with a color
void Fill() {
  word firstLed = ReceiveWord();

  word numberOfLeds = ReceiveWord();

  int ColorData = ReceiveColorData();

  if ( firstLed <= configuredStripLength * configuredStrips && numberOfLeds > 0 && firstLed + numberOfLeds - 1 <= configuredStripLength * configuredStrips ) {
    word endLedNr = firstLed + numberOfLeds;
    for (word ledNr = firstLed; ledNr < endLedNr; ledNr++) {
      leds[ledNr] = CRGB(ColorData);
    }
    Ack();
  } else {
    //Number of the first led or the number of leds to receive is outside the allowed range
    Nack();
  }


}


//Receives data for the ledstrips
void ReceiveData() {
  word firstLed = ReceiveWord();

  word numberOfLeds = ReceiveWord();

  if ( firstLed <= configuredStripLength * configuredStrips && numberOfLeds > 0 && firstLed + numberOfLeds - 1 <= configuredStripLength * configuredStrips ) {
    //FirstLedNr and numberOfLeds are valid.
    //Receive and set color data

    word endLedNr = firstLed + numberOfLeds;
    for (word ledNr = firstLed; ledNr < endLedNr; ledNr++) {
      leds[ledNr] = CRGB(ReceiveColorData());
    }

    Ack();

  } else {
    //Number of the first led or the number of leds to receive is outside the allowed range
    Nack();
  }
}

//Sets the length of the longest connected ledstrip. Length is restricted to the max number of allowed leds
void SetLedStripLength() {
  word stripLength = ReceiveWord();
  if (stripLength < 1 || stripLength > MaxLedsPerStrip) {
    //stripLength is either to small or above the max number of leds allowed
    Nack();
  } else {
    //stripLength is in the valid range
    configuredStripLength = stripLength;
// Removed until i figure out to change the length dynamicly    
//    SetFastLedStripes(stripLength);
    Ack();
  }
}

//Clears the data for all configured leds
void  ClearAllLedData() {
  FastLED.clear();
//  fill_solid( leds, MaxLedsPerStrip*configuredStrips, CRGB(0,0,0));
  Ack();
}


//Sends the firmware version
void SendVersion() {
  Serial.write(FirmwareVersionMajor);
  Serial.write(FirmwareVersionMinor);
  Ack();
}

//Sends the max number of leds per strip
void SendMaxNumberOfLeds() {
  byte B = MaxLedsPerStrip >> configuredStrips;
  Serial.write(B);
  B = MaxLedsPerStrip & 255;
  Serial.write(B);
  Ack();
}


//Sends a ack (A)
void Ack() {
  Serial.write('A');
}

//Sends a NACK (N)
void Nack() {
  Serial.write('N');
}

//Receives 3 bytes of color data.
int ReceiveColorData() {
  while (!Serial.available()) {};
  int colorValue = Serial.read();
  while (!Serial.available()) {};
  colorValue = (colorValue << 8) | Serial.read();
  while (!Serial.available()) {};
  colorValue = (colorValue << 8) | Serial.read();

  return colorValue;


}

//Receives a word value. High byte first, low byte second
word ReceiveWord() {
  while (!Serial.available()) {};
  word wordValue = Serial.read() << 8;
  while (!Serial.available()) {};
  wordValue = wordValue | Serial.read();

  return wordValue;
}

void SetFastLedStripes(int NumOfLeds) {
  FastLED.addLeds<NEOPIXEL, 14>(leds, 0, NumOfLeds);
  FastLED.addLeds<NEOPIXEL, 15>(leds, NumOfLeds, NumOfLeds);
  FastLED.addLeds<NEOPIXEL, 26>(leds, 2 * NumOfLeds, NumOfLeds);
  FastLED.addLeds<NEOPIXEL, 27>(leds, 3 * NumOfLeds, NumOfLeds);
  FastLED.addLeds<NEOPIXEL, 19>(leds, 4 * NumOfLeds, NumOfLeds);
  FastLED.addLeds<NEOPIXEL, 16>(leds, 5 * NumOfLeds, NumOfLeds);
}
