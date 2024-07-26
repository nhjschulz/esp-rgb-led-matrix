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
 * @brief  View for 64x64 LED matrix with date and time.
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "DateTimeView64x64.h"

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

/** 
  * Sinus lookup table for analog clock drawing
  * 
  * Holds sinus values for the minutes 0 .. 15 angles in quadarant 0.
  * Other quadrants and cosinus values get derived from these values 
  * to avoid recalculations.
  */
static const float MINUTE_SINUS_TABLE[16u] = {
    0.0f,                    /* sin(0°)   */
    0.10452846326765346f,    /* sin(6°)   */
    0.20791169081775931f,    /* sin(12°)  */
    0.3090169943749474f,     /* sin(18°)  */
    0.40673664307580015f,    /* sin(24°)  */
    0.49999999999999994f,    /* sin(30°)  */
    0.5877852522924731f,     /* sin(36°)  */
    0.6691306063588582f,     /* sin(42°)  */
    0.7431448254773941f,     /* sin(48°)  */
    0.8090169943749475f,     /* sin(54°)  */
    0.8660254037844386f,     /* sin(60°)  */
    0.9135454576426009f,     /* sin(66°)  */
    0.9510565162951535f,     /* sin(72°)  */
    0.9781476007338056f,     /* sin(78°)  */
    0.9945218953682733f,     /* sin(84°)  */
    1.0f                     /* sin(90°)  */
};

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/**
 * Update the underlying canvas.
 *
 * @param[in] gfx   Graphic functionality to draw on the underlying canvas.
 */
void DateTimeView64x64::update(YAGfx& gfx)
{
    DateTimeViewGeneric::update(gfx);

    /* Draw analog clock minute circle*/
    drawAnalogClockBackground(gfx);

    /* Draw analog clck hands. */
    drawAnalogClockHand(gfx, m_now.tm_hour * 5 + m_now.tm_min / 12 , ANALOG_RADIUS - 12, ColorDef::GRAY);
    drawAnalogClockHand(gfx, m_now.tm_min, ANALOG_RADIUS - 6, ColorDef::GRAY);
    drawAnalogClockHand(gfx, m_now.tm_sec, ANALOG_RADIUS - 1, ColorDef::YELLOW);
    
    /* Draw analog clock hand center */
    gfx.drawRectangle(ANALOG_CENTER_X - 2, ANALOG_CENTER_Y - 2, 5, 5, ColorDef::YELLOW);
    gfx.drawPixel(ANALOG_CENTER_X, ANALOG_CENTER_Y, ColorDef::BLACK);
    gfx.drawPixel(ANALOG_CENTER_X-2, ANALOG_CENTER_Y-2, ColorDef::BLACK);
    gfx.drawPixel(ANALOG_CENTER_X-2, ANALOG_CENTER_Y+2, ColorDef::BLACK);
    gfx.drawPixel(ANALOG_CENTER_X+2, ANALOG_CENTER_Y-2, ColorDef::BLACK);
    gfx.drawPixel(ANALOG_CENTER_X+2, ANALOG_CENTER_Y+2, ColorDef::BLACK);



}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

float DateTimeView64x64::getMinuteSinus(uint16_t angle)
{
    float sinus = 0.0;

    angle %= 360u; 

    /* 
     * Lookup table only stores 1st quadrant sinus values. 
     * Others are calculated based on sinus curve symetries.
     */

    if (90u >= angle)  /* quadrant 1 */
    {
        sinus = MINUTE_SINUS_TABLE[angle / 6u];
    }
    else if (180u >= angle) /* quadrant 2 is symetric to quadrant 1*/
    {
        sinus = MINUTE_SINUS_TABLE[(180u - angle) / 6u];
    }
    else if ( 270u >= angle)  /* quadrant 3 is point symetric to 2 */
    {
        sinus = (-1.0) * MINUTE_SINUS_TABLE[(angle - 180u) / 6u];
    }
    else /* quadrant 4 is symetric to 3 */
    {
        sinus = (-1.0) * MINUTE_SINUS_TABLE[(360 - angle) / 6u];
    }

    return sinus;
}

float DateTimeView64x64::getMinuteCosinus(uint16_t angle)
{
    return getMinuteSinus(angle + 90u);
}

void DateTimeView64x64::drawAnalogClockBackground(YAGfx& gfx)
{
    /* draw minute ring */

    for (uint16_t angle = 0u; angle < 360u; angle += 6u)
    {
        float dx = getMinuteCosinus(angle);
        float dy = getMinuteSinus(angle);

        float xs = ANALOG_CENTER_X + dx * ANALOG_RADIUS;
        float ys = ANALOG_CENTER_Y + dy * ANALOG_RADIUS;

        if (0u == (angle % 30u))  /* Draw stronger marks at 3,6,9,12 hour */
        {
            float xe = ANALOG_CENTER_X + dx * (ANALOG_RADIUS - 4);
            float ye = ANALOG_CENTER_Y + dy * (ANALOG_RADIUS - 4);
            gfx.drawLine(xs, ys, xe, ye, ColorDef::BLUE);
        }
        else
        {
            gfx.drawPixel(xs, ys, ColorDef::DARKGRAY);
        }
    }

}

void DateTimeView64x64::drawAnalogClockHand(YAGfx& gfx, int16_t minute, int16_t radius, const Color& col)
{
    /* convert minute to angle starting at 270° which draws towards the top of the clock.*/
    minute %= 60;
    minute = 270 + minute * 6;

    float dx = getMinuteCosinus(minute);
    float dy = getMinuteSinus(minute);

     gfx.drawLine(
        ANALOG_CENTER_X + 3 * dx,
        ANALOG_CENTER_Y + 3 * dy,
        ANALOG_CENTER_X + radius * dx,
        ANALOG_CENTER_Y + radius * dy,
        col);
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
