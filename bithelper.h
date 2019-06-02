#pragma once

#define lowByte(w) ((w) & 0xFF)
#define highByte(w) (((w) >> 8) & 0xFF)
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#define bit(b) (1UL << (b))
#define bitToggle(value, bit) ((value) ^= 1UL << bit)