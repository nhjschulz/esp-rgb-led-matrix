/* MIT License
 *
 * Copyright (c) 2019 - 2025 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @brief  WiFi utilities
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "WiFiUtil.h"
#include <Esp.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/******************************************************************************
 * External Functions
 *****************************************************************************/

extern uint8_t WiFiUtil::getSignalQuality(int8_t rssi)
{
    uint8_t         signalQuality       = 0U;
    const int8_t    RSSI_INVALID        = 0;    /* Invalid dBm value */
    const int8_t    RSSI_HIGH           = -50;  /* dBm */
    const int8_t    RSSI_UNUSABLE       = -100; /* dBm */
    const uint8_t   SIGNAL_QUALITY_FULL = 100U; /* % */
    const uint8_t   SIGNAL_QUALITY_BAD  = 0U;   /* % */
    const uint8_t   CONVERSION_FACTOR   = 100U / static_cast<uint8_t>(RSSI_HIGH - RSSI_UNUSABLE);

    if (RSSI_INVALID == rssi)
    {
        signalQuality = SIGNAL_QUALITY_BAD;
    }
    else if (RSSI_HIGH <= rssi)
    {
        signalQuality = SIGNAL_QUALITY_FULL;
    }
    else if (RSSI_UNUSABLE >= rssi)
    {
        signalQuality = SIGNAL_QUALITY_BAD;
    }
    else
    {
        signalQuality = static_cast<uint8_t>(rssi - RSSI_HIGH) * CONVERSION_FACTOR;
    }

    return signalQuality;
}

extern void WiFiUtil::getEFuseMAC(String& macAddr)
{
    uint64_t    efuseMAC        = ESP.getEfuseMac();
    uint8_t     byte1           = (efuseMAC >> 40U) & 0xffU;
    uint8_t     byte2           = (efuseMAC >> 32U) & 0xffU;
    uint8_t     byte3           = (efuseMAC >> 24U) & 0xffU;
    uint8_t     byte4           = (efuseMAC >> 16U) & 0xffU;
    uint8_t     byte5           = (efuseMAC >>  8U) & 0xffU;
    uint8_t     byte6           = (efuseMAC >>  0U) & 0xffU;
    size_t      BUFFER_SIZE     = 18U;
    char        buffer[BUFFER_SIZE];

    (void)snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", byte6, byte5, byte4, byte3, byte2, byte1);

    macAddr = buffer;
}

extern void WiFiUtil::getChipId(String& chipId)
{
    uint64_t    efuseMAC        = ESP.getEfuseMac();
    int32_t     highPart        = (efuseMAC >> 8U) & 0x0000ffffU;
    int32_t     lowPart         = (efuseMAC >> 0U) & 0xffffffffU;
    size_t      BUFFER_SIZE     = 13U;
    char        buffer[BUFFER_SIZE];

    (void)snprintf(buffer, sizeof(buffer), "%04X%08X", highPart, lowPart);

    chipId = buffer;
}

/******************************************************************************
 * Local Functions
 *****************************************************************************/
