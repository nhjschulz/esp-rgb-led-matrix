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
 * @brief  View with icon, text and progress bar for 64x64 LED matrix
 * @author Andreas Merkle <web@blue-andi.de>
 * @addtogroup PLUGIN
 *
 * @{
 */

#ifndef PLAYER_VIEW_64X64_H
#define PLAYER_VIEW_64X64_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <YAGfx.h>
#include <Fonts.h>
#include <IPlayerView.h>
#include <BitmapWidget.h>
#include <TextWidget.h>
#include <ProgressBar.h>
#include <Util.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * View for 64x64 LED matrix with icon, text and progress bar.
 */
class PlayerView64x64 : public IPlayerView
{
public:

    /**
     * Construct the view.
     */
    PlayerView64x64() :
        IPlayerView(),
        m_fontType(Fonts::FONT_TYPE_DEFAULT),
        m_bitmapWidget(BITMAP_WIDTH, BITMAP_HEIGHT, BITMAP_X, BITMAP_Y),
        m_textWidget(TEXT_WIDTH, TEXT_HEIGHT, TEXT_X, TEXT_Y),
        m_progressBar(PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, PROGRESS_BAR_X, PROGRESS_BAR_Y)
    {
        m_bitmapWidget.setHorizontalAlignment(Alignment::Horizontal::HORIZONTAL_CENTER);
        m_bitmapWidget.setVerticalAlignment(Alignment::Vertical::VERTICAL_CENTER);
    }

    /**
     * Destroy the view.
     */
    virtual ~PlayerView64x64()
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

    /**
     * Set progress in % [0; 100].
     * 
     * @param[in] progress  Progress as number from 0 to 100.
     */
    void setProgress(uint8_t progress) override
    {
        m_progressBar.setProgress(progress);
    }

protected:

    /**
     * Bitmap size in pixels.
     */
    static const uint16_t   BITMAP_SIZE         = CONFIG_LED_MATRIX_HEIGHT / 2U;

    /**
     * Bitmap width in pixels.
     */
    static const uint16_t   BITMAP_WIDTH        = BITMAP_SIZE;

    /**
     * Bitmap height in pixels.
     */
    static const uint16_t   BITMAP_HEIGHT       = BITMAP_SIZE;

    /**
     * Bitmap widget x-coordinate in pixels.
     * Left aligned.
     */
    static const int16_t    BITMAP_X            = (CONFIG_LED_MATRIX_WIDTH - BITMAP_WIDTH) / 2;

    /**
     * Bitmap widget y-coordinate in pixels.
     * Top aligned.
     */
    static const int16_t    BITMAP_Y            = 0;

    /**
     * Text width in pixels.
     */
    static const uint16_t   TEXT_WIDTH          = CONFIG_LED_MATRIX_WIDTH;

    /**
     * Text height in pixels.
     */
    static const uint16_t   TEXT_HEIGHT         = CONFIG_LED_MATRIX_HEIGHT - BITMAP_HEIGHT - 2U;

    /**
     * Text widget x-coordinate in pixels.
     */
    static const int16_t    TEXT_X              = 0;

    /**
     * Text widget y-coordinate in pixels.
     * Top aligned, below bitmap.
     */
    static const int16_t    TEXT_Y              = CONFIG_LED_MATRIX_HEIGHT - BITMAP_HEIGHT + 2;

    /**
     * Progress bar width in pixels.
     */
    static const uint16_t   PROGRESS_BAR_WIDTH  = CONFIG_LED_MATRIX_WIDTH;

    /**
     * Progress bar height in pixels.
     */
    static const uint16_t   PROGRESS_BAR_HEIGHT = 2U;

    /**
     * Progress bar x-coordinate in pixels.
     */
    static const int16_t    PROGRESS_BAR_X      = 0;

    /**
     * Progress bar y-coordinate in pixels.
     */
    static const int16_t    PROGRESS_BAR_Y      = CONFIG_LED_MATRIX_HEIGHT - BITMAP_HEIGHT;

    Fonts::FontType m_fontType;     /**< Font type which shall be used if there is no conflict with the layout. */
    BitmapWidget    m_bitmapWidget; /**< Bitmap widget used to show a icon. */
    TextWidget      m_textWidget;   /**< Text widget used to show some text. */
    ProgressBar     m_progressBar;  /**< Progress bar for the music. */

private:
    PlayerView64x64(const PlayerView64x64& other);
    PlayerView64x64& operator=(const PlayerView64x64& other);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* PLAYER_VIEW_64X64_H */

/** @} */
