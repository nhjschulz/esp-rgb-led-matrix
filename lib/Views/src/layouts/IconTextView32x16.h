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
 * @brief  View with icon and text for 32x16 LED matrix
 * @author Andreas Merkle <web@blue-andi.de>
 * @addtogroup PLUGIN
 *
 * @{
 */

#ifndef ICON_TEXT_VIEW_32X16_H
#define ICON_TEXT_VIEW_32X16_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <YAGfx.h>
#include <Fonts.h>
#include <IIconTextView.h>
#include <BitmapWidget.h>
#include <TextWidget.h>
#include <Util.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * View for 32x16 LED matrix with icon and text.
 * 
 * +-----------------------------------------------------------------+
 * |                |                                                |
 * |                |                                                |
 * |                |                                                |
 * |   Icon         |                   Text                         |
 * |   8x16         |                   24x16                        |
 * |                |                                                |
 * |                |                                                |
 * |                |                                                |
 * +-----------------------------------------------------------------+
 */
class IconTextView32x16 : public IIconTextView
{
public:

    /**
     * Construct the view.
     */
    IconTextView32x16() :
        IIconTextView(),
        m_fontType(Fonts::FONT_TYPE_DEFAULT),
        m_bitmapWidget(BITMAP_WIDTH, BITMAP_HEIGHT, BITMAP_X, BITMAP_Y),
        m_textWidget(TEXT_WIDTH, TEXT_HEIGHT, TEXT_X, TEXT_Y)
    {
        m_bitmapWidget.setVerticalAlignment(Alignment::Vertical::VERTICAL_CENTER);
        m_bitmapWidget.setHorizontalAlignment(Alignment::Horizontal::HORIZONTAL_CENTER);
        
        m_textWidget.setVerticalAlignment(Alignment::Vertical::VERTICAL_CENTER);
    }

    /**
     * Destroy the view.
     */
    virtual ~IconTextView32x16()
    {
    }

    /**
     * Initialize view, which will prepare the widgets and the default values.
     * 
     * @param[in] width     Display width in pixel.
     * @param[in] height    Display height in pixel.
     */
    void init(uint16_t width, uint16_t height) override
    {
        UTIL_NOT_USED(width);
        UTIL_NOT_USED(height);
    }

    /**
     * Get font type.
     * 
     * @return The font type the view uses.
     */
    Fonts::FontType getFontType() const override
    {
        return m_fontType;
    }

    /**
     * Set font type.
     * 
     * @param[in] fontType  The font type which the view shall use.
     */
    void setFontType(Fonts::FontType fontType) override
    {
        m_fontType = fontType;
        m_textWidget.setFont(Fonts::getFontByType(m_fontType));
    }

    /**
     * Update the underlying canvas.
     * 
     * @param[in] gfx   Graphic functionality to draw on the underlying canvas.
     */
    void update(YAGfx& gfx) override
    {
        gfx.fillScreen(ColorDef::BLACK);
        m_bitmapWidget.update(gfx);
        m_textWidget.update(gfx);
    }

    /**
     * Get text (non-formatted).
     * 
     * @return Text
     */
    String getText() const override
    {
        return m_textWidget.getStr();
    }

    /**
     * Get text (formatted).
     * 
     * @return Text
     */
    String getFormatText() const override
    {
        return m_textWidget.getFormatStr();
    }

    /**
     * Set text (formatted).
     * 
     * @param[in] formatText    Formatted text to show.
     */
    void setFormatText(const String& formatText) override
    {
        m_textWidget.setFormatStr(formatText);
    }

    /**
     * Load icon image from filesystem.
     *
     * @param[in] filename  Image filename
     *
     * @return If successul, it will return true otherwise false.
     */
    bool loadIcon(const String& filename) override;

    /**
     * Clear icon.
     */
    void clearIcon() override
    {
        m_bitmapWidget.clear(ColorDef::BLACK);
    }

protected:

    /**
     * Bitmap width in pixels.
     */
    static const uint16_t   BITMAP_WIDTH    = 8U;

    /**
     * Bitmap height in pixels.
     */
    static const uint16_t   BITMAP_HEIGHT   = CONFIG_LED_MATRIX_HEIGHT;

    /**
     * Bitmap widget x-coordinate in pixels.
     * Left aligned.
     */
    static const int16_t    BITMAP_X        = 0;

    /**
     * Bitmap widget y-coordinate in pixels.
     * Top aligned.
     */
    static const int16_t    BITMAP_Y        = 0;

    /**
     * Text width in pixels.
     */
    static const uint16_t   TEXT_WIDTH      = CONFIG_LED_MATRIX_WIDTH - BITMAP_WIDTH;

    /**
     * Text height in pixels.
     */
    static const uint16_t   TEXT_HEIGHT     = CONFIG_LED_MATRIX_HEIGHT;

    /**
     * Text widget x-coordinate in pixels.
     */
    static const int16_t    TEXT_X          = BITMAP_WIDTH;

    /**
     * Text widget y-coordinate in pixels.
     * Top aligned, below bitmap.
     */
    static const int16_t    TEXT_Y          = 0;

    Fonts::FontType m_fontType;     /**< Font type which shall be used if there is no conflict with the layout. */
    BitmapWidget    m_bitmapWidget; /**< Bitmap widget used to show a icon. */
    TextWidget      m_textWidget;   /**< Text widget used to show some text. */

private:
    IconTextView32x16(const IconTextView32x16& other);
    IconTextView32x16& operator=(const IconTextView32x16& other);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* ICON_TEXT_VIEW_32X16_H */

/** @} */
