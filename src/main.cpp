#include <Arduino.h>

#define SOFT_START_PIN 9    // connected to 15
#define SPEED_CONFIG1_PIN 2 // connected to 0
#define SPEED_CONFIG2_PIN 6 // connected to 4

#define RX_START_BYTE 2
byte rxBuffer[50];
byte rxBufferIndex = 0;
byte rxDataLen = 0;
byte rxHash = 0;

byte driverMode = 2; // 0-2
byte passMode = 15;
byte startUpMode = 128; // 128-192
byte currentLimit = 12; // 1-50
byte wheelInch = 20;
byte softStartSpeed = 0;
bool softStartEnabled = true;

double periodMS = 0;
double speed = 0;

#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

uint32_t txMillis = 0;

void sendSettings(byte maxSpeed)
{
  if (millis() - txMillis < 100)
    return;
  txMillis = millis();
  byte txBuffer[20] = {1, 20, 1, driverMode, passMode, startUpMode, 46, 0, wheelInch * 10, 1, 0, 0, maxSpeed, currentLimit, 1, 164, 0, 0, 5, 0};
  byte hash = 0;
  for (byte i = 0; i < 19; i++)
  {
    hash ^= txBuffer[i];
  }
  txBuffer[19] = hash;
  for (byte i = 0; i < 20; i++)
  {
    mySerial.write(txBuffer[i]);
  }
}

bool rxRoutine()
{
  while (Serial.available() > 0)
  {
    byte rxByte = Serial.read();
    if (rxBufferIndex == 0)
    {
      if (rxByte == RX_START_BYTE)
      {
        rxBuffer[rxBufferIndex++] = rxByte;
        rxDataLen = 0;
        rxHash = 0;
      }
    }
    else if (rxBufferIndex == 1)
    {
      rxBuffer[rxBufferIndex++] = rxByte;
      rxDataLen = rxByte;
      if (rxByte != 14)
        rxBufferIndex = 0;
    }
    else if (rxBufferIndex > 1 && rxBufferIndex + 1 < rxDataLen)
    {
      rxBuffer[rxBufferIndex++] = rxByte;
    }
    else
    {
      rxBuffer[rxBufferIndex++] = rxByte;
      for (byte i = 0; i < rxDataLen - 1; i++)
      {
        rxHash ^= rxBuffer[i];
      }
      rxBufferIndex = 0;

      if (rxHash == rxByte)
      {
        //        for (byte i = 0 ; i < rxDataLen - 1 ; i++) {
        //          Serial.print(rxBuffer[i]);
        //          Serial.print("-");
        //
        //        }
        //        Serial.println(rxHash);
        return true;
      }
      else
      {
        Serial.println("hash error");
        for (byte i = 0; i < rxDataLen; i++)
        {
          Serial.print(rxBuffer[i]);
          Serial.print("-");
        }
      }
    }
  }
  return false;
}

void setup()
{
  Serial.begin(9600);
  mySerial.begin(9600);
  pinMode(SOFT_START_PIN, INPUT_PULLUP);
  pinMode(SPEED_CONFIG1_PIN, INPUT_PULLUP);
  pinMode(SPEED_CONFIG2_PIN, INPUT_PULLUP);
}

void onRxData()
{
  periodMS = ((uint16_t)(rxBuffer[8]) << 8) | rxBuffer[9];
  if (periodMS > 3000)
    speed = 0;
  else
    speed = 2.54 * (double)wheelInch * 3.14 * 6.0 * 6.0 / (periodMS);

  Serial.println(speed);
}

uint32_t softStartMillis = 0;
void calculateSoftStart()
{
  if (millis() - softStartMillis < 100)
    return;
  softStartMillis = millis();
  softStartSpeed = speed + (speed < 20 ? 15 : 20);
  if (softStartSpeed < 5)
    softStartSpeed = 5;
}

int maxSpeed;

void readConfig()
{
  softStartEnabled = !digitalRead(SOFT_START_PIN);
  if (digitalRead(SPEED_CONFIG1_PIN) && digitalRead(SPEED_CONFIG2_PIN))
    maxSpeed = 20;
  else if (digitalRead(SPEED_CONFIG1_PIN) && !digitalRead(SPEED_CONFIG2_PIN))
    maxSpeed = 40;
  else
    maxSpeed = 100;
}
uint32_t ledMillis = 0;
bool ledStatus = false;
void loop()
{
  if (rxRoutine())
    onRxData();
  calculateSoftStart();
  readConfig();

  if (softStartEnabled)
    sendSettings(softStartSpeed > maxSpeed ? maxSpeed : softStartSpeed);
  else
    sendSettings(maxSpeed);

  if (millis() - ledMillis > 200)
  {
    digitalWrite(13, ledStatus = !ledStatus);
    ledMillis = millis();
  }
}
