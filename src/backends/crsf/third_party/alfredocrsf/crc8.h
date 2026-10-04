// SPDX-License-Identifier: GPL-3.0-only
// Adapted from AlfredoSystems/AlfredoCRSF as recorded in README.md.
#pragma once

#include <stdint.h>

class Crc8 {
  public:
    Crc8(uint8_t poly);
    uint8_t calc(uint8_t *data, uint8_t len);

    // Compute the CRC bit by bit to avoid building a lookup table.
    static uint8_t calcPoly(const uint8_t *data, uint8_t len, uint8_t poly);

  protected:
    uint8_t _lut[256];
    void init(uint8_t poly);
};