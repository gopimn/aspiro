
/*LIBRARIES*/
#include <SPI.h> //Para poder comunicarse con 
#include <SD.h>
#include "DHT.h"
#include <Wire.h>
#include "RTClib.h"

/*GLOBAL DECLARATIONS*/
#define A 0.0039083
#define B 0.0000005775
#define C -0.000000000004183
const int DTRDY = 7;      //indicates the end of data conversion
const int ADS_CS = 9;     //Comunication control pin
const int chipSelect = 8;
int regarray[14];         //Saves all the registers of the ads1247
int RTDA = 0, RTDB = 0;
double RT;
/*  SETUP*/
void setup() {
  Serial.begin(9600);
  //  while (!Serial) {
  //    ; // wait for serial port to connect. Needed for Leonardo only
  //  }
  pinMode(ADS_CS, OUTPUT);
  digitalWrite(ADS_CS, HIGH);
  pinMode(DTRDY, INPUT);
  SPI.begin();

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
  else
    return value;
}
/* END OF FUNCTIONS*/


/*     MAIN LOOP    */
void loop() {
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
  ads_write_reg(0x3, 0x31);
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

    /*
      >> A=3.9083*10^-3
      A =  0.0039083
      >> B=-5.775*10^-7
      B =   -5.7750e-07
      >> C=-4.183*10^-12
      C =   -4.1830e-12
      >> (-A+sqrt(A^2-4*B*(1-150/100)))/(2*B)
      ans =  130.45
    */

    ads_write_reg(0x0, 0x01);
    ads_write_reg(0xB, 0x01);
    RTDA = ads_read_once();
    delay(10);
    ads_write_reg(0x0, 0x1A);
    ads_write_reg(0xB, 0x23);
    RTDB = ads_read_once();
    delay(10);
    RT = (((c_t(RTDA) * 2.7) / (8388607)) );
    Serial.print(RT, DEC);
    Serial.print("::");
    Serial.print(RTDA, HEX);
    Serial.print("::");
    Serial.print(RTDA, DEC);
    Serial.print("::");
    Serial.print(c_t(RTDA), DEC);
    Serial.print("::");
    RT = (((c_t(RTDB) * 2.7) / (8388607))) ;
    Serial.print(RT, DEC);
    Serial.println();
    delay(1000);
  }
}
