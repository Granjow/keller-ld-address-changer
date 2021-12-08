#include "Arduino.h"
#include <Wire.h>

#define LD_OLD_ADDR 61
#define LD_NEW_ADDR 62

#define LD_ADDR_MASK 0b1111111

#define COMMAND_MODE 0xa9

// aka Start_NOM
#define NORMAL_MODE 0xa8

struct BitChanges {
    uint8_t toOne;
    uint8_t toZero;
};

int countOneBits(uint16_t input) {
    int nonZeroBits = 0;
    for (int i = 0; i < 16; i++) {
        nonZeroBits += input & 1;
        input = input >> 1;
    }
    return nonZeroBits;
}

bool isValidAddress(int address) {
    if (address >= 0x00 && address <= 0x07) {
        Serial.println(F("Invalid address: 0x00-0x07 are reserved"));
        return false;
    }
    if (address != (address & LD_ADDR_MASK)) {
        Serial.println(F("Invalid address. Only 7 LSBs are allowed. Max is 127 (0x7f)."));
        return false;
    }
    if (address >= 0x78 && address <= 0x7f) {
        Serial.println(F("Addresses between 0x78 and 0x7f are not recommended."));
        return true;
    }
    return true;
}

BitChanges checkBitChanges(uint8_t oldAddress, uint8_t newAddress) {
    BitChanges bc{};
    for (int i = 0; i < 7; i++) {
        bool oldBit = oldAddress & 1;
        bool newBit = newAddress & 1;
        if (oldBit && !newBit) {
            bc.toZero++;
        } else if (!oldBit && newBit) {
            bc.toOne++;
        }
    }
    return bc;
}

void writeNewAddress(uint8_t i2cAddress, uint8_t newAddress) {
    Wire.beginTransmission(i2cAddress);
    uint8_t addressUpdate[] = {0x42, 0x00, newAddress};
    Wire.write(addressUpdate, 3);
    Wire.endTransmission();
}

uint8_t readAddress(uint8_t i2cAddress) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x02);
    delay(1);

    Wire.requestFrom(LD_OLD_ADDR, 3);

    Wire.read(); // status
    Wire.read(); // hiAddr
    uint8_t loAddress = Wire.read();
    Wire.endTransmission();

    return loAddress;
}

void enterNormalMode(uint8_t i2cAddress) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(NORMAL_MODE);
    Wire.endTransmission();
}

void setup() {
    uint8_t oldAddress = LD_OLD_ADDR;
    uint8_t newAddress = LD_NEW_ADDR;

    Serial.println(F("Address Changer for Keller LD sensors ready."));
    Serial.print(F("Current address: 0x"));
    Serial.print(oldAddress, HEX);
    Serial.print(F("\nNew address: 0x"));
    Serial.print(newAddress, HEX);
    Serial.print(F("\nBinary form:\nOld: "));
    Serial.print(oldAddress, BIN);
    Serial.print(F("\nNew: "));
    Serial.print(newAddress, BIN);
    Serial.println();

    /////////////////
    // Address checks
    /////////////////

    if (!isValidAddress(newAddress)) {
        return;
    }

    auto bc = checkBitChanges(oldAddress, newAddress);
    if (bc.toZero > 0) {
        Serial.print(F("Error: "));
        Serial.print(bc.toZero);
        Serial.print(F(" bits change to 0 with the new address.\n"));
        Serial.print(F("The LD Sensor memory can only burn 1s but not erase them.\nChoose a different address.\n"));
        return;
    }

    if (bc.toOne > 1) {
        Serial.print(F("WARNING: "));
        Serial.print(bc.toOne);
        Serial.print(F(" bits will be changed to 1s. They cannot be erased anymore! "));
    }

    uint8_t currentAddress = readAddress(oldAddress);

    /////////////////
    // Change address
    /////////////////

    Serial.print(F("Current address in senor RAM: 0x"));
    Serial.print(currentAddress, HEX);

    Serial.print(F("\nChange to new address 0x"));
    Serial.print(newAddress, HEX);
    Serial.print(F("? (y/n) > "));
    auto response = Serial.readString();

    if (response != "y") {
        Serial.println("Not changing address.");
        return;
    }


    Serial.println("Would write new address now!");
    // (not actually writing for now)
    // writeNewAddress(oldAddress, newAddress);

    enterNormalMode(oldAddress);

    uint8_t updatedAddress = readAddress(oldAddress);
    Serial.print(F("New address in RAM is 0x"));
    Serial.print(updatedAddress, HEX);
}

void loop() {
    delay(1000);
}
