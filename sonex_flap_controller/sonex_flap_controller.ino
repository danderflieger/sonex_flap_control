/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// #define SCREEN_WIDTH 128 // OLED display width, in pixels
// #define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

int ANGLE1 = 0;
int ANGLE2 = 10;
int ANGLE3 = 25;
int TITLE_MARGIN = 8;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



void setup() {
  Serial.begin(9600);

  Serial.println("Serial started ...");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  // display.display();
  // delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.setRotation(1);

  drawTitle();

  drawReferenceLine(ANGLE1);
  drawReferenceValue(ANGLE1);

  drawReferenceLine(ANGLE2);
  drawReferenceValue(ANGLE2);

  drawReferenceLine(ANGLE3);
  drawReferenceValue(ANGLE3);


  // display.display();

}

void loop() {
  
  int SensorReading = getTestAngleData();
  drawNewAngle(SensorReading);
  Serial.println(SensorReading, DEC);
  
}

void drawReferenceLine(int angle) {
  
  display.fillRect(23, getYPosition(angle)+TITLE_MARGIN, 13, 5, SSD1306_WHITE);
  display.display();

}

void drawTitle() {

  char textValues[] = "FLAPS";

  display.setTextSize(2);
  display.fillRect(0, 0, SCREEN_HEIGHT, 16, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(3, 1);
  display.cp437(true);
  display.write(textValues);
  display.display();

}

void drawReferenceValue(int angle) {

  char textValues[3];
  sprintf(textValues, "%d", angle);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, getYPosition(angle)+ 3);
  display.cp437(true);
  display.write(textValues);
  display.display();

  Serial.println(angle);

}



void drawNewAngle(int angle) {

  display.fillRect(0, getYPosition(ANGLE1), 22, SCREEN_WIDTH, SSD1306_BLACK);
  display.fillRect(2, getYPosition(angle) + TITLE_MARGIN, 18, 5, SSD1306_WHITE);
  display.display();
  delay(10);

}

int getYPosition(int angle) {
  int newValue = map(angle, 0, ANGLE3 + 5, 20, SCREEN_WIDTH);
  // Serial.print(angle); Serial.print(": "); Serial.println(newValue);
  return newValue;
  
}

int testAngle = 0;
bool countUp = true;

int getTestAngleData() {

  if (testAngle <= ANGLE1) {
    countUp = true;
  }
  if (testAngle >= ANGLE3) {
    countUp = false;
  }

  if (countUp) {
    testAngle = testAngle + 2;
  } else {
    testAngle = testAngle - 2;
  }
  delay(100);
  // Serial.println(testAngle);
  return testAngle;
  
}

// void testdrawchar(void) {
//   display.clearDisplay();

//   display.setTextSize(1);      // Normal 1:1 pixel scale
//   display.setTextColor(SSD1306_WHITE); // Draw white text
//   display.setCursor(0, 0);     // Start at top-left corner
//   display.cp437(true);         // Use full 256 char 'Code Page 437' font

//   // Not all the characters will fit on the display. This is normal.
//   // Library will draw what it can and the rest will be clipped.
//   for(int16_t i=0; i<256; i++) {
//     if(i == '\n') display.write(' ');
//     else          display.write(i);
//   }

//   display.display();
//   delay(2000);
// }



// void testdrawrect(void) {
//   display.clearDisplay();

//   for(int16_t i=0; i<display.height()/2; i+=2) {
//     display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
//     display.display(); // Update screen with each newly-drawn rectangle
//     delay(1);
//   }

//   delay(2000);
// }

// void testfillrect(void) {
//   display.clearDisplay();

//   for(int16_t i=0; i<display.height()/2; i+=3) {
//     // The INVERSE color is used so rectangles alternate white/black
//     display.fillRect(i, i, display.width()-i*2, display.height()-i*2, SSD1306_INVERSE);
//     display.display(); // Update screen with each newly-drawn rectangle
//     delay(1);
//   }

//   delay(2000);
// }


// void testdrawroundrect(void) {
//   display.clearDisplay();

//   for(int16_t i=0; i<display.height()/2-2; i+=2) {
//     display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
//       display.height()/4, SSD1306_WHITE);
//     display.display();
//     delay(1);
//   }

//   delay(2000);
// }

// void testfillroundrect(void) {
//   display.clearDisplay();

//   for(int16_t i=0; i<display.height()/2-2; i+=2) {
//     // The INVERSE color is used so round-rects alternate white/black
//     display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
//       display.height()/4, SSD1306_INVERSE);
//     display.display();
//     delay(1);
//   }

//   delay(2000);
// }

// void testdrawtriangle(void) {
//   display.clearDisplay();

//   for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
//     display.drawTriangle(
//       display.width()/2  , display.height()/2-i,
//       display.width()/2-i, display.height()/2+i,
//       display.width()/2+i, display.height()/2+i, SSD1306_WHITE);
//     display.display();
//     delay(1);
//   }

//   delay(2000);
// }

// void testfilltriangle(void) {
//   display.clearDisplay();

//   for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
//     // The INVERSE color is used so triangles alternate white/black
//     display.fillTriangle(
//       display.width()/2  , display.height()/2-i,
//       display.width()/2-i, display.height()/2+i,
//       display.width()/2+i, display.height()/2+i, SSD1306_INVERSE);
//     display.display();
//     delay(1);
//   }

//   delay(2000);
// }



// void testdrawstyles(void) {
//   display.clearDisplay();

//   display.setTextSize(1);             // Normal 1:1 pixel scale
//   display.setTextColor(SSD1306_WHITE);        // Draw white text
//   display.setCursor(0,0);             // Start at top-left corner
//   display.println(F("Hello, world!"));

//   display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
//   display.println(3.141592);

//   display.setTextSize(2);             // Draw 2X-scale text
//   display.setTextColor(SSD1306_WHITE);
//   display.print(F("0x")); display.println(0xDEADBEEF, HEX);

//   display.display();
//   delay(2000);
// }

// void testscrolltext(void) {
//   display.clearDisplay();

//   display.setTextSize(2); // Draw 2X-scale text
//   display.setTextColor(SSD1306_WHITE);
//   display.setCursor(10, 0);
//   display.println(F("scroll"));
//   display.display();      // Show initial text
//   delay(100);

//   // Scroll in various directions, pausing in-between:
//   display.startscrollright(0x00, 0x0F);
//   delay(2000);
//   display.stopscroll();
//   delay(1000);
//   display.startscrollleft(0x00, 0x0F);
//   delay(2000);
//   display.stopscroll();
//   delay(1000);
//   display.startscrolldiagright(0x00, 0x07);
//   delay(2000);
//   display.startscrolldiagleft(0x00, 0x07);
//   delay(2000);
//   display.stopscroll();
//   delay(1000);
// }

// void testdrawbitmap(void) {
//   display.clearDisplay();

//   display.drawBitmap(
//     (display.width()  - LOGO_WIDTH ) / 2,
//     (display.height() - LOGO_HEIGHT) / 2,
//     logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
//   display.display();
//   delay(1000);
// }

// #define XPOS   0 // Indexes into the 'icons' array in function below
// #define YPOS   1
// #define DELTAY 2

// void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
//   int8_t f, icons[NUMFLAKES][3];

//   // Initialize 'snowflake' positions
//   for(f=0; f< NUMFLAKES; f++) {
//     icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
//     icons[f][YPOS]   = -LOGO_HEIGHT;
//     icons[f][DELTAY] = random(1, 6);
//     Serial.print(F("x: "));
//     Serial.print(icons[f][XPOS], DEC);
//     Serial.print(F(" y: "));
//     Serial.print(icons[f][YPOS], DEC);
//     Serial.print(F(" dy: "));
//     Serial.println(icons[f][DELTAY], DEC);
//   }

//   for(;;) { // Loop forever...
//     display.clearDisplay(); // Clear the display buffer

//     // Draw each snowflake:
//     for(f=0; f< NUMFLAKES; f++) {
//       display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, SSD1306_WHITE);
//     }

//     display.display(); // Show the display buffer on the screen
//     delay(200);        // Pause for 1/10 second

//     // Then update coordinates of each flake...
//     for(f=0; f< NUMFLAKES; f++) {
//       icons[f][YPOS] += icons[f][DELTAY];
//       // If snowflake is off the bottom of the screen...
//       if (icons[f][YPOS] >= display.height()) {
//         // Reinitialize to a random position, just off the top
//         icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
//         icons[f][YPOS]   = -LOGO_HEIGHT;
//         icons[f][DELTAY] = random(1, 6);
//       }
//     }
//   }
// }
