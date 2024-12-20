/* MIT License
 *
 * Copyright (c) 2019 - 2024 Andreas Merkle <web@blue-andi.de>
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
 * @brief  View for 32x16 LED matrix with date and time.
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "DateTimeView32x16.h"

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

/* Initialize the color of the actual day. */
const Color DateTimeView32x16::DAY_ON_COLOR  = ColorDef::LIGHTGRAY;

/* Initialize the color of the other days (not the actual day). */
const Color DateTimeView32x16::DAY_OFF_COLOR = ColorDef::ULTRADARKGRAY;

/******************************************************************************
 * Public Methods
 *****************************************************************************/

void DateTimeView32x16::setCurrentTime(const tm& now)
{
    /* update lamp widgets */

    /* tm_wday starts at sunday, first lamp specified via m_startOfWeek.*/
    uint8_t activeLamp = now.tm_wday - m_startOfWeek;

    /* the above subtraction may underflow, so detect and correct for that.
     * this is more efficient than integer remainder on xtensa cores. */
    if (MAX_LAMPS < activeLamp)
    {
        activeLamp += MAX_LAMPS;
    }

    uint8_t index;

    for(index = 0U; index < MAX_LAMPS; ++index)
    {
        m_lampWidgets[index].setOnState(index == activeLamp);
    }
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

void DateTimeView32x16::updateLampWidgetsColors()
{
    uint8_t index;

    for(index = 0U; index < MAX_LAMPS; ++index)
    {
        m_lampWidgets[index].setColorOn(m_dayOnColor);
        m_lampWidgets[index].setColorOff(m_dayOffColor);
    }
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
