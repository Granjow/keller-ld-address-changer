#include "Arduino.h"
#include <Wire.h>

#define LD_OLD_ADDR 61
#define LD_NEW_ADDR 63

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
        bool oldBit = (oldAddress >> i) & 1;
        bool newBit = (newAddress >> i) & 1;
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

void enterCommandMode(uint8_t i2cAddress){
    Wire.beginTransmission(i2cAddress);
    Wire.write(COMMAND_MODE);
    Wire.endTransmission();
}

void enterNormalMode(uint8_t i2cAddress) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(NORMAL_MODE);
    Wire.endTransmission();
}

void printBitChanges(BitChanges changes) {
    Serial.print(changes.toZero);
    Serial.print(F(" to 0, "));
    Serial.print(changes.toOne);
    Serial.print(F(" to 1\n"));
}

// Poor man's unit tests :)
bool runUnitTests() {
    bool success = true;

    auto changes = checkBitChanges(0b0000000, 0b1111111);
    if (changes.toOne != 7 || changes.toZero != 0) {
        printBitChanges(changes);
        Serial.print("Bit checker check 1 failed\n");
        success = false;
    }

    changes = checkBitChanges(0b1111000, 0b0000111);
    if (changes.toOne != 3 || changes.toZero != 4) {
        printBitChanges(changes);
        Serial.print("Bit checker check 2 failed\n");
        success = false;
    }

    return success;
}

void setup() {
    Serial.begin(38400);
    Serial.println(F("Address Changer for Keller LD sensors initialising ..."));

    uint8_t oldAddress = LD_OLD_ADDR;
    uint8_t newAddress = LD_NEW_ADDR;

    auto unitTestsSucceeded = runUnitTests();
    if (!unitTestsSucceeded) {
        Serial.print(F("Self test failed. Quitting.\n"));
        return;
    }


    Serial.println(F("Address Changer for Keller LD sensors ready."));
    Serial.print(F("Current address: 0x"));
    Serial.print(oldAddress, HEX);
    Serial.print(F("\nNew address: 0x"));
    Serial.print(newAddress, HEX);
    Serial.print(F("\nBinary form:\n"));
    Serial.print(oldAddress, BIN);
    Serial.print(F(" (old)\n"));
    Serial.print(newAddress, BIN);
    Serial.print(F(" (new)\n"));

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
    enterCommandMode(oldAddress);
    // writeNewAddress(oldAddress, newAddress);

    // Entering normal mode does not update the address.
    enterNormalMode(oldAddress);

    uint8_t updatedAddress = readAddress(oldAddress);
    Serial.print(F("New address in RAM is 0x"));
    Serial.print(updatedAddress, HEX);
}

void loop() {
    delay(1000);
}
