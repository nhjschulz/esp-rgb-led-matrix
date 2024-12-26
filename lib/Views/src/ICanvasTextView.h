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
 * @brief  Canvas and text view interface
 * @author Andreas Merkle <web@blue-andi.de>
 * @addtogroup plugin
 *
 * @{
 */

#ifndef ICANVAS_TEXT_VIEW_H
#define ICANVAS_TEXT_VIEW_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <YAGfx.h>
#include <WString.h>
#include <Fonts.h>
#include <YAGfxCanvas.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Interface for a view with a canvas and text.
 */
class ICanvasTextView
{
public:

    /**
     * Destroy the interface.
     */
    virtual ~ICanvasTextView()
    {
    }

    /**
     * Initialize view, which will prepare the widgets and the default values.
     */
    virtual void init(uint16_t width, uint16_t height) = 0;

    /**
     * Get font type.
     * 
     * @return The font type the view uses.
     */
    virtual Fonts::FontType getFontType() const = 0;

    /**
     * Set font type.
     * 
     * @param[in] fontType  The font type which the view shall use.
     */
    virtual void setFontType(Fonts::FontType fontType) = 0;

    /**
     * Update the underlying canvas.
     * 
     * @param[in] gfx   Graphic functionality to draw on the underlying canvas.
     */
    virtual void update(YAGfx& gfx) = 0;

    /**
     * Get text (non-formatted).
     * 
     * @return Text
     */
    virtual String getText() const = 0;

    /**
     * Get text (formatted).
     * 
     * @return Text
     */
    virtual String getFormatText() const = 0;

    /**
     * Set text (formatted).
     * 
     * @param[in] formatText    Formatted text to show.
     */
    virtual void setFormatText(const String& formatText) = 0;

    /**
     * Get canvas for drawing.
     * 
     * @return Canvas
     */
    virtual YAGfx& getCanvasGfx() = 0;

protected:

    /**
     * Construct the interface.
     */
    ICanvasTextView()
    {
    }

};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* ICANVAS_TEXT_VIEW_H */

/** @} */
