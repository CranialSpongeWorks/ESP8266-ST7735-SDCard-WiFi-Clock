/***************************************************************************
 file:<ESP8266 ST7735 SDCard WiFi Clock.ino>
 ST7735 Clock Display with SD Card Digit Images
  
 Board: LOLIN(WEMOS) D1 mini

 Created by David C. Boyce
 Youtube link: https://www.youtube.com/@PCBoardRepair
 GitHub link: https://github.com/CranialSpongeWorks/ESP8266-ST7735-SDCard-WiFi-Clock

****************************************************************************/
#include <Adafruit_GFX.h>    // include Adafruit graphics library
#include <Adafruit_ST7735.h> // include Adafruit ST7735 display library
#include <SPI.h>             // include Arduino SPI library
#include <SD.h>              // include Arduino SD library
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char ssid[] = "SSID";  //  your network SSID (name)
const char pass[] = "PASSWORD";       // your network password

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
const int timeZone = -5;  // Eastern Standard Time (USA)

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

// define SD Card connections
#define SD_CS    D8   // chip select line
//SD_SCK         D5   // 
//SD_MOSI        D7   //
//SD_MISO        D6   //
//SD_VCC         +5V  //
//SD_GND         GND  //

// define ST7735 TFT display connections
//TFT_SCK        D5   //
//TFT_SDA        D7   //
//TFT_RES        RST  //
//TFT_BL         N.C. //
#define TFT_DC   D0   // data/command line
#define TFT_RST  -1   // reset line (optional, pass -1 if not used)
#define TFT1_CS  D1   // chip select line
#define TFT2_CS  D2   // chip select line
#define TFT3_CS  D3   // chip select line
#define TFT4_CS  D4   // chip select line

// initialize Adafruit ST7735 TFT library
Adafruit_ST7735 tft1 = Adafruit_ST7735(TFT1_CS, TFT_DC, TFT_RST);
Adafruit_ST7735 tft2 = Adafruit_ST7735(TFT2_CS, TFT_DC, TFT_RST);
Adafruit_ST7735 tft3 = Adafruit_ST7735(TFT3_CS, TFT_DC, TFT_RST);
Adafruit_ST7735 tft4 = Adafruit_ST7735(TFT4_CS, TFT_DC, TFT_RST);

// TFT Update Speed
#define BUFFPIXEL 75

time_t getNtpTime();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Needed for Leonardo only
  delay(1000);

  // setup OUTPUT pins
  pinMode(TFT1_CS, OUTPUT);
  digitalWrite(TFT1_CS, HIGH);
  pinMode(TFT2_CS, OUTPUT);
  digitalWrite(TFT2_CS, HIGH);
  pinMode(TFT3_CS, OUTPUT);
  digitalWrite(TFT3_CS, HIGH);
  pinMode(TFT4_CS, OUTPUT);
  digitalWrite(TFT4_CS, HIGH);

  // initialize ST7735S TFT display
  Serial.println();
  Serial.print("Initializing ST7735S LCD modules...");
  tft1.initR(INITR_BLACKTAB);
  tft1.fillScreen(ST77XX_BLUE);
  tft2.initR(INITR_BLACKTAB);
  tft2.fillScreen(ST77XX_GREEN);
  tft3.initR(INITR_BLACKTAB);
  tft3.fillScreen(ST77XX_YELLOW);
  tft4.initR(INITR_BLACKTAB);
  tft4.fillScreen(ST77XX_RED);
  Serial.println("OK!");

  //Initialze SD Card
  Serial.println();
  Serial.print("Initializing SD card...");
  tft1.setTextColor(ST77XX_WHITE);
  tft1.setCursor(5, 10);
  tft1.println("Initializing...");
  tft1.setCursor(5, 20);
  tft1.print("SD Card: ");
  if (!SD.begin(SD_CS)) {
    Serial.print("failed!");
    tft1.println("failed!");
    while(1);  // stay here
  }
  Serial.println("OK!");
  tft1.println("OK!");
  File root = SD.open("/");  // open SD card main root
  printDirectory(root, 0);   // print all files names and sizes
  root.close();              // close the opened root

  //Initialze Wifi
  Serial.println();
  Serial.print("Connecting to ");
  tft1.setCursor(5, 40);
  tft1.println("Connecting to");
  Serial.print(ssid);
  tft1.setCursor(5, 50);
  tft1.println(ssid);
  WiFi.begin(ssid, pass);
  tft1.setCursor(5, 60);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tft1.print(".");
  }
  tft1.println();
  tft1.setCursor(5, 70);
  Serial.print("Connected!!!");
  Serial.println();
  tft1.println("Connected!!!");
  Serial.print("IP number assigned by DHCP is ");
  tft1.setCursor(5, 90);
  tft1.println(WiFi.localIP());
  Serial.println(WiFi.localIP());
  tft1.setCursor(5, 100);
  tft1.println("Starting UDP");
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  tft1.setCursor(5, 110);
  tft1.println(Udp.localPort());
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  tft1.setCursor(5, 130);
  tft1.println("Ready!");
  delay(2000);
}

time_t  prevDisplay = 0;  // when the digital clock was displayed

void loop() {
 //Time Retrieval
 if (timeStatus() != timeNotSet) {
  if (now() != prevDisplay) { //update the display only if time has changed
   prevDisplay = now();
  }
 }

 // output TIME to Serial Monitor
 Serial.println();
 print2Digits(hour());
 Serial.print(":");
 print2Digits(minute());
 Serial.print(":");
 print2Digits(second());
 Serial.print(" ");
 print2Digits(month());
 Serial.print("/");
 print2Digits(day());
 Serial.print("/");
 Serial.print(year());
 Serial.println();

 // process LCD displays to show Time
 update_LCD();
// delay(100);
}

void update_LCD(){
 File bmp_file; char* file_name = "";
 int tmphour = hour(); int tmphour10 = 0; int tmphour1 = 0;
 int tmpmin = minute(); int tmpmin10 = 0; int tmpmin1 = 0;
 if (tmphour >= 12){tmphour = tmphour-12;}; if (tmphour == 0 ){tmphour = 12;}
 tmphour = DecToBCD(tmphour); tmphour10 = tmphour >>4; tmphour1 = tmphour & 0x0F;
 tmpmin = DecToBCD(tmpmin); tmpmin10 = tmpmin >>4; tmpmin1 = tmpmin & 0x0F;

 if (isPM()){
  //PM
  //digit 1
  switch (tmphour10) {
   case 0: 
    file_name="0PM.bmp";
    break;
   case 1: 
    file_name="1PM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw1(file_name,0,0);
  bmp_file.close(); 

  //digit 2
  switch (tmphour1) {
   case 0: 
    file_name="0PM.bmp";
    break;
   case 1: 
    file_name="1PM.bmp";
    break;
   case 2: 
    file_name="2PM.bmp";
    break;
   case 3: 
    file_name="3PM.bmp";
    break;
   case 4: 
    file_name="4PM.bmp";
    break;
   case 5: 
    file_name="5PM.bmp";
    break;
   case 6: 
    file_name="6PM.bmp";
    break;
   case 7: 
    file_name="7PM.bmp";
    break;
   case 8: 
    file_name="8PM.bmp";
    break;
   case 9: 
    file_name="9PM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw2(file_name,0,0);
  bmp_file.close(); 

  //digit 3
  switch (tmpmin10) {
   case 0: 
    file_name="0PM.bmp";
    break;
   case 1: 
    file_name="1PM.bmp";
    break;
   case 2: 
    file_name="2PM.bmp";
    break;
   case 3: 
    file_name="3PM.bmp";
    break;
   case 4: 
    file_name="4PM.bmp";
    break;
   case 5: 
    file_name="5PM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw3(file_name,0,0);
  bmp_file.close(); 

  //digit 4
  switch (tmpmin1){
   case 0: 
    file_name="0PM.bmp";
    break;
   case 1: 
    file_name="1PM.bmp";
    break;
   case 2: 
    file_name="2PM.bmp";
    break;
   case 3: 
    file_name="3PM.bmp";
    break;
   case 4: 
    file_name="4PM.bmp";
    break;
   case 5: 
    file_name="5PM.bmp";
    break;
   case 6: 
    file_name="6PM.bmp";
    break;
   case 7: 
    file_name="7PM.bmp";
    break;
   case 8: 
    file_name="8PM.bmp";
    break;
   case 9: 
    file_name="9PM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw4(file_name,0,0);
  bmp_file.close(); 

 } else {  
  //AM
  //digit 1
  switch (tmphour10) {
   case 0: 
    file_name="0AM.bmp";
    break;
   case 1: 
    file_name="1AM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw1(file_name,0,0);
  bmp_file.close(); 

  //digit 2
  switch (tmphour1) {
   case 0: 
    file_name="0AM.bmp";
    break;
   case 1: 
    file_name="1AM.bmp";
    break;
   case 2: 
    file_name="2AM.bmp";
    break;
   case 3: 
    file_name="3AM.bmp";
    break;
   case 4: 
    file_name="4AM.bmp";
    break;
   case 5: 
    file_name="5AM.bmp";
    break;
   case 6: 
    file_name="6AM.bmp";
    break;
   case 7: 
    file_name="7AM.bmp";
    break;
   case 8: 
    file_name="8AM.bmp";
    break;
   case 9: 
    file_name="9AM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw2(file_name,0,0);
  bmp_file.close(); 

  //digit 3
  switch (tmpmin10) {
   case 0: 
    file_name="0AM.bmp";
    break;
   case 1: 
    file_name="1AM.bmp";
    break;
   case 2: 
    file_name="2AM.bmp";
    break;
   case 3: 
    file_name="3AM.bmp";
    break;
   case 4: 
    file_name="4AM.bmp";
    break;
   case 5: 
    file_name="5AM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw3(file_name,0,0);
  bmp_file.close(); 

  //digit 4
  switch (tmpmin1){
   case 0: 
    file_name="0AM.bmp";
    break;
   case 1: 
    file_name="1AM.bmp";
    break;
   case 2: 
    file_name="2AM.bmp";
    break;
   case 3: 
    file_name="3AM.bmp";
    break;
   case 4: 
    file_name="4AM.bmp";
    break;
   case 5: 
    file_name="5AM.bmp";
    break;
   case 6: 
    file_name="6AM.bmp";
    break;
   case 7: 
    file_name="7AM.bmp";
    break;
   case 8: 
    file_name="8AM.bmp";
    break;
   case 9: 
    file_name="9AM.bmp";
    break;
  }
  bmp_file = SD.open(file_name);
  if(!bmp_file) {
   Serial.println();
   Serial.print("Didn't find BMPimage!");
  }
  bmpDraw4(file_name,0,0);
  bmp_file.close(); 

 } //if isPM()
} //updateLCD(void)

void print2Digits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime(){
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address){
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.
void bmpDraw1(char *filename, uint8_t x, uint16_t y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft1.width()) || (y >= tft1.height())) return;

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.println(); Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft1.width())  w = tft1.width()  - x;
        if((y+h-1) >= tft1.height()) h = tft1.height() - y;

        // Set TFT address window to clipped image bounds
        tft1.startWrite();
        tft1.setAddrWindow(x, y, w, h);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            tft1.endWrite();
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
              tft1.startWrite();
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft1.pushColor(tft1.color565(r,g,b));
          } // end pixel
        } // end scanline
        tft1.endWrite();
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

void bmpDraw2(char *filename, uint8_t x, uint16_t y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft2.width()) || (y >= tft2.height())) return;

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.println(); Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft2.width())  w = tft2.width()  - x;
        if((y+h-1) >= tft2.height()) h = tft2.height() - y;

        // Set TFT address window to clipped image bounds
        tft2.startWrite();
        tft2.setAddrWindow(x, y, w, h);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            tft2.endWrite();
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
              tft2.startWrite();
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft2.pushColor(tft2.color565(r,g,b));
          } // end pixel
        } // end scanline
        tft2.endWrite();
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

void bmpDraw3(char *filename, uint8_t x, uint16_t y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft3.width()) || (y >= tft3.height())) return;

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.println(); Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft3.width())  w = tft3.width()  - x;
        if((y+h-1) >= tft3.height()) h = tft3.height() - y;

        // Set TFT address window to clipped image bounds
        tft3.startWrite();
        tft3.setAddrWindow(x, y, w, h);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            tft3.endWrite();
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
              tft3.startWrite();
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft3.pushColor(tft3.color565(r,g,b));
          } // end pixel
        } // end scanline
        tft3.endWrite();
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

void bmpDraw4(char *filename, uint8_t x, uint16_t y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft4.width()) || (y >= tft4.height())) return;

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.println(); Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft4.width())  w = tft4.width()  - x;
        if((y+h-1) >= tft4.height()) h = tft4.height() - y;

        // Set TFT address window to clipped image bounds
        tft4.startWrite();
        tft4.setAddrWindow(x, y, w, h);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            tft4.endWrite();
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
              tft4.startWrite();
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft4.pushColor(tft4.color565(r,g,b));
          } // end pixel
        } // end scanline
        tft4.endWrite();
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }
  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void prntBits(byte b) {
  for(int i = 7; i >= 0; i--)
    Serial.print(bitRead(b,i));
  Serial.println();  
}

byte DecToBCD(byte dec){
	byte result = 0;
	result |= (dec / 10) << 4;
	result |= (dec % 10) << 0;
	return result;
}

//end