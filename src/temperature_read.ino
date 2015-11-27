/*LIBRARIES*/
#include <SPI.h>
#include <SD.h>
#include "DHT.h"
#include <Wire.h>
#include "RTClib.h"


/*GLOBAL DECLARATIONS*/
const int DTRDY = 7;      //indicates the end of data conversion
const int ADS_CS = 9;     //Comunication control pin
const int chipSelect = 8;
int regarray[14];         //Saves all the registers of the ads1247
signed long RTDA, RTDB;

RTC_DS1307 rtc;
int mode_index = 0;
unsigned long ttime;
Ds1307SqwPinMode modes[] = {OFF, ON, SquareWave1HZ, SquareWave4kHz, SquareWave8kHz, SquareWave32kHz};


#define DHTPIN 5
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

/*  SETUP*/
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode(ADS_CS, OUTPUT);
  digitalWrite(ADS_CS, HIGH);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);
  pinMode(DTRDY, INPUT);

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  SPI.begin();
  dht.begin();
  Wire.begin();
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

/* BEGIN OF FUNCTIONS*/

signed long ads_get_reg_value(int reg) { //falta verificar que sean 4 bites
  int response;
  int command = 0x2;
  command <<= 4;
  command |= reg;          //Se genera el comando de lectura
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(ADS_CS, LOW);
  SPI.transfer(command);   //Se comunica el comando
  SPI.transfer(0x00);      //cantidad de bytes a leer - 1
  response = SPI.transfer(0xFF);
  digitalWrite(ADS_CS, HIGH);
  SPI.endTransaction();
  return response;
}

void ads_get_all_regs() {
  for (int i = 0; i <= 14; i++)
    regarray[i] = ads_get_reg_value(i); //SE DEBEN MEZCLAR ESTAS FUNCIONES!?!?!?!
}

void ads_print_regarray() {
  String msj;
  for (int i = 0; i <= 13; i++) {
    msj = "0x0" + String(i, HEX) + " -> " + String(regarray[i], HEX);
    Serial.println(msj);
  }
}//search for streaming library

void ads_write_reg(int reg, int value) { // falta verificar los tamaños de los imputs
  int command = 0x4;
  command <<= 4;
  command |= reg;          //Se genera el comando de escritura
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(ADS_CS, LOW);
  SPI.transfer(command);   //Se comunica el comando
  SPI.transfer(0x00);      //cantidad de bytes a escribir - 1
  SPI.transfer(value);
  digitalWrite(ADS_CS, HIGH);
  SPI.endTransaction();
}

void ads_reset() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(ADS_CS, LOW);
  SPI.transfer(0x07);
  delay(10);
  digitalWrite(ADS_CS, HIGH);
  SPI.endTransaction();
}

long ads_read_once() {
  while (1) {
    if (digitalRead(DTRDY) == LOW) {
      signed long reading = 0x0;
      SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
      digitalWrite(ADS_CS, LOW);   //CS negative logic
      SPI.transfer(0x13);          //Rdata once
      reading |= SPI.transfer(0xFF);// Get the first byte
      reading <<= 8;                // add bytes to total reading
      reading |= SPI.transfer(0xFF);// Get the second byte
      reading <<= 8;
      reading |= SPI.transfer(0xFF);// and so on...
      digitalWrite(ADS_CS, HIGH);
      SPI.endTransaction();
      return reading;
    }
  }
}

signed long c_t(long value) {
  if (value > 8388607) {
    value = value - 16777216;
  }
  return value;
}
/* END OF FUNCTIONS*/


/*     MAIN LOOP    */
void loop() {
  DateTime now = rtc.now();
  unsigned long stamp, pstamp = millis();
  int samplecounter = 0;
  Serial.println("RESET TO POWERUP VALUES");
  ads_reset();
  ads_get_all_regs();
  ads_print_regarray();
  delay(1000);
  Serial.println("APPLY CONFIGURATION TO ADC");
  ads_write_reg(0x0, 0x01);
  ads_write_reg(0x1, 0x00);
  ads_write_reg(0x2, 0x20);
  ads_write_reg(0x3, 0x01);
  ads_write_reg(0x4, 0x00);
  ads_write_reg(0x5, 0x00);
  ads_write_reg(0x6, 0x00);
  ads_write_reg(0xA, 0x07);
  ads_write_reg(0xB, 0x01);
  ads_get_all_regs();
  ads_print_regarray();
  Serial.print("2 sec programed delay");
  while (samplecounter < 2) {
    Serial.print(".");
    delay(1000);
    samplecounter++;
  }
  Serial.println();
  while (1) {
    now = rtc.now();
    pinMode(ADS_CS, OUTPUT);
    digitalWrite(ADS_CS, HIGH);
    String dataString = "";
    RTDA = ads_read_once();
    dataString += String((273*c_t(RTDA)/64267.85)-273);
    ads_write_reg(0x0, 0x1A);
    ads_write_reg(0xB, 0x23);
    delay(10);
    dataString += ";";
    RTDB = ads_read_once();
    dataString += String((273*c_t(RTDB)/80500.1)-273);
    ads_write_reg(0x0, 0x01);
    ads_write_reg(0xB, 001);
    dataString += ";";
    dataString += String(now.hour());
    dataString += ";";
    dataString += String(now.minute());
    dataString += ";";
    dataString += String(now.second());
    delay(10);
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    else {
      Serial.println("error opening datalog.txt");
    }
    Serial.print(((c_t(RTDA)*0.000074)+5.5), DEC);
    Serial.print("::");
    Serial.print(((c_t(RTDB)*0.000073)+6.5), DEC);
    Serial.print("::");
    float t = dht.readTemperature();
    Serial.print(t);
    Serial.print("::");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    delay(1000);
  }
}
