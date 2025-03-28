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
 * @brief  GIF image player
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "GifImgPlayer.h"
#include "LzwDecoder.h"
#include "GifFileLoader.h"
#include "GifFileToMemLoader.h"

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/** Value to disable internal debug mode. */
#define GIF_IMG_PLAYER_DEBUG_DISABLE    0

/** Value to enable internal debug mode. */
#define GIF_IMG_PLAYER_DEBUG_ENABLE     1

/** Debug mode status */
#define GIF_IMG_PLAYER_DEBUG_MODE       GIF_IMG_PLAYER_DEBUG_DISABLE

/******************************************************************************
 * Macros
 *****************************************************************************/

#if GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE

/** Debug log macro. */
#define GIF_IMG_PLAYER_LOG_DEBUG(...)       printf(__VA_ARGS__)

/** Reset the frame counter to zero. */
#define GIF_IMG_PLAYER_RESET_FRAME_COUNT()  do{ gFrameCnt = 0u; }while(0)

/** Increase the frame counter by one. */
#define GIF_IMG_PLAYER_INC_FRAME_COUNT()    do{ ++gFrameCnt; }while(0)

#else /* GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE */

#define GIF_IMG_PLAYER_LOG_DEBUG(...)
#define GIF_IMG_PLAYER_RESET_FRAME_COUNT()
#define GIF_IMG_PLAYER_INC_FRAME_COUNT()

#endif /* GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE */

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/** GIF signature */
static const char*  GIF_SIGNATURE   = "GIF"; /* 3 byte signature, incl. string termination. */

/** Supported GIF version 87a */
static const char*  GIF_VERSION_87A = "87a"; /* 3 byte version, incl. string termination. */

/** Supported GIF version 89a */
static const char*  GIF_VERSION_89A = "89a"; /* 3 byte version, incl. string termination. */

/**
 * The main block ids.
 */
typedef enum : uint8_t
{
    BLOCK_ID_EXTENSION          = 0x21U,    /**< Extension */
    BLOCK_ID_IMAGE_DESCRIPTOR   = 0x2CU,    /**< Image descriptor */
    BLOCK_ID_TRAILER            = 0x3BU     /**< Trailer */

} BlockId;

/**
 * Supported extension labels which gives the GIF parser the information about
 * the kind of extension.
 */
typedef enum : uint8_t
{
    LABEL_PLAIN_TEXT_EXTENSION      = 0x01U,    /**< Plain text extension */
    LABEL_GRAPHIC_CONTROL_EXTENSION = 0xF9U,    /**< Graphic control extension */
    LABEL_COMMENT_EXTENSION         = 0xFEU,    /**< Comment extension */
    LABEL_APPLICATION_EXTENSION     = 0xFFU     /**< Application extension */

} ExtensionLabel;

/**
 * To store general information about the GIF image file.
 * Not needed after the file is loaded in memory.
 */
typedef struct _GifFileHeader
{
    uint8_t signature[3U];  /**< Signature */
    uint8_t version[3U];    /**< Version */

} __attribute__ ((packed)) GifFileHeader;

/**
 * Packed field out of the logical screen descriptor.
 */
typedef struct _LsdPackedField
{
    uint8_t globalColorTableSizeExp : 3;    /**< Size exponent (2^(N+1)) of global color table */
    uint8_t sortFlag : 1;                   /**< Sort flag */
    uint8_t colorResolution : 3;            /**< Color resolution */
    uint8_t globalColorTableFlag : 1;       /**< Global color table flag */

} __attribute__ ((packed)) LsdPackedField;

/**
 * The logical screen descriptor.
 */
typedef struct _LogicalScreenDescriptor
{
    uint16_t        canvasWidth;        /**< Canvas width in pixel */
    uint16_t        canvasHeight;       /**< Canvas height in pixel */
    LsdPackedField  packedField;        /**< Packed field */
    uint8_t         bgColorIndex;       /**< Background color index */
    uint8_t         pixelAspectRatio;   /**< Pixel aspect ratio */

} __attribute__ ((packed)) LogicalScreenDescriptor;

/**
 * Packed field out of the graphic control extension.
 */
typedef struct _GcePackedField
{
    uint8_t transparentColorFlag : 1;   /**< Transparent color flag */
    uint8_t userInputFlag : 1;          /**< User input flag */
    uint8_t disposalMethod : 3;         /**< Disposal method */
    uint8_t reserved : 3;               /**< Reserved for future use */

} __attribute__ ((packed)) GcePackedField;

/**
 * The graphic control extension.
 */
typedef struct _GraphicControlExtension
{
    GcePackedField  packedField;            /**< Packed field */
    uint16_t        delayTime;              /**< Delay time in 1/100 s */
    uint8_t         transparentColorIndex;  /**< Transparent color index */

} __attribute__ ((packed)) GraphicControlExtension;

/**
 * Packed field out of the image descriptor.
 */
typedef struct _IdPackedField
{
    uint8_t localColorTableSizeExp : 3;     /**< Size exponent (2^(N+1)) of local color table */
    uint8_t reserved : 2;                   /**< Reserved for future use */
    uint8_t sortFlag : 1;                   /**< Sort flag */
    uint8_t interlaceFlag : 1;              /**< Interlace flag */
    uint8_t localColorTableFlag : 1;        /**< Local color table flag */

} __attribute__ ((packed)) IdPackedField;

/**
 * The image descriptor.
 */
typedef struct _ImageDescriptor
{
    uint16_t        imageLeft;      /**< x-coordinate inside the canvas. */
    uint16_t        imageTop;       /**< y-coordinate inside the canvas. */
    uint16_t        imageWidth;     /**< Image width in pixel. */
    uint16_t        imageHeight;    /**< Image height in pixel. */
    IdPackedField   packedField;    /**< Packed field */

} __attribute__ ((packed)) ImageDescriptor;

/**
 * The application extension.
 */
typedef struct _ApplicationExtension
{
    uint8_t identifier[8U];         /**< Application identifier */
    uint8_t authenticationCode[3U]; /**< Application authentication code */

}  __attribute__ ((packed)) ApplicationExtension;

/******************************************************************************
 * Prototypes
 *****************************************************************************/

#if GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE
static const char* getDisposalMethodAsString(uint8_t disposalMethod);
#endif /* GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE */

/******************************************************************************
 * Local Variables
 *****************************************************************************/

#if GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE
/** Frame counter (counts the image descriptor) */
static uint32_t gFrameCnt = 0U;
#endif /* GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE */

/******************************************************************************
 * Public Methods
 *****************************************************************************/

GifImgPlayer::Ret GifImgPlayer::open(FS& fs, const String& fileName, bool toMem)
{
    Ret ret = RET_OK;
    
    /* File already opened? */
    if (nullptr != m_gifLoader)
    {
        ret = RET_FILE_ALREADY_OPENED;
    }
    /* Open file */
    else
    {
        if (false == toMem)
        {
            m_gifLoader = new(std::nothrow) GifFileLoader;
        }
        else
        {
            m_gifLoader = new(std::nothrow) GifFileToMemLoader;
        }

        if (nullptr == m_gifLoader)
        {
            ret = RET_IMG_TOO_BIG;
        }
        else if (false == m_gifLoader->open(fs, fileName))
        {
            ret = RET_FILE_NOT_FOUND;
        }
        else
        {
            /* Allocate image data block, used for parsing. */
            m_imageDataBlock = new(std::nothrow) uint8_t[IMAGE_DATA_BLOCK_SIZE];

            if (nullptr == m_imageDataBlock)
            {
                ret = RET_FILE_FORMAT_UNSUPPORTED;
            }
        }
    }

    /* If file is opened, the parsing will be started. */
    if (RET_OK == ret)
    {
        GifFileHeader           gifFileHeader;
        LogicalScreenDescriptor logicalScreenDescriptor;

        /* Read the GIF file header. */
        if (false == m_gifLoader->read(&gifFileHeader, sizeof(gifFileHeader)))
        {
            ret = RET_FILE_FORMAT_INVALID;
        }
        /* Is it not a supported GIF file? */
        else if (false == isFileSupported(gifFileHeader))
        {
            ret = RET_FILE_FORMAT_UNSUPPORTED;
        }
        /* Read the logical screen descriptor. */
        else if (false == m_gifLoader->read(&logicalScreenDescriptor, sizeof(logicalScreenDescriptor)))
        {
            ret = RET_FILE_FORMAT_INVALID;
        }
        else
        {
            GIF_IMG_PLAYER_LOG_DEBUG("Logical screen descriptor\n");
            GIF_IMG_PLAYER_LOG_DEBUG("\tCanvas width           : %u\n", logicalScreenDescriptor.canvasWidth);
            GIF_IMG_PLAYER_LOG_DEBUG("\tCanvas height          : %u\n", logicalScreenDescriptor.canvasHeight);
            GIF_IMG_PLAYER_LOG_DEBUG("\tPacked field\n");
            GIF_IMG_PLAYER_LOG_DEBUG("\t\tGlobal color table size exponent: %u\n", logicalScreenDescriptor.packedField.globalColorTableSizeExp);
            GIF_IMG_PLAYER_LOG_DEBUG("\t\tSort flag                       : %u\n", logicalScreenDescriptor.packedField.sortFlag);
            GIF_IMG_PLAYER_LOG_DEBUG("\t\tColor resolution                : %u\n", logicalScreenDescriptor.packedField.colorResolution);
            GIF_IMG_PLAYER_LOG_DEBUG("\t\tGlobal color table flag         : %u\n", logicalScreenDescriptor.packedField.globalColorTableFlag);
            GIF_IMG_PLAYER_LOG_DEBUG("\tBackground color index : %u\n", logicalScreenDescriptor.bgColorIndex);
            GIF_IMG_PLAYER_LOG_DEBUG("\tPixel aspect ratio     : %u\n", logicalScreenDescriptor.pixelAspectRatio);
            GIF_IMG_PLAYER_LOG_DEBUG("\tGlobal color table size: %u colors\n", calcColorTableSize(logicalScreenDescriptor.packedField.globalColorTableSizeExp) / 3U);

            m_bgColorIndex = logicalScreenDescriptor.bgColorIndex;
            
            /* Create the bitmap as small as possible to not waste memory. */
            if (false == m_bitmap.create(logicalScreenDescriptor.canvasWidth, logicalScreenDescriptor.canvasHeight))
            {
                ret = RET_IMG_TOO_BIG;
            }
            else
            {
                /* Reset */
                m_disposalMethod        = DISPOSAL_METHOD_NO_ACTION;
                m_isTrailerFound        = false;
                m_loopCount             = 0U;
                m_delay                 = 0U;
                m_isTransparencyEnabled = false;
                m_isAnimation           = false;
                m_isFinished            = false;
                m_timer.stop();

                /* Global color table available? */
                if (0U != logicalScreenDescriptor.packedField.globalColorTableFlag)
                {
                    size_t globalColorTableSize = calcColorTableSize(logicalScreenDescriptor.packedField.globalColorTableSizeExp);
                    
                    m_globalColorTableLength    =  globalColorTableSize / sizeof(PaletteColor);
                    m_globalColorTable          = new(std::nothrow) PaletteColor[m_globalColorTableLength];

                    /* Out of memory? */
                    if (nullptr == m_globalColorTable)
                    {
                        m_globalColorTableLength = 0U;
                        ret = RET_IMG_TOO_BIG;
                    }
                    /* Read global color table. */
                    else if (false == m_gifLoader->read(m_globalColorTable, globalColorTableSize))
                    {
                        ret = RET_FILE_FORMAT_INVALID;
                    }
                    else
                    {
                        ;
                    }
                }
            }
        }
    }

    /* Clean up in case a error happended. */
    if (RET_OK != ret)
    {
        cleanup();
    }

    return ret;
}

void GifImgPlayer::close()
{
    cleanup();
}

bool GifImgPlayer::play(YAGfx& gfx, int16_t x, int16_t y)
{
    bool isSuccessful = true;

    /* Reset trailer status. */
    m_isTrailerFound = false;

    /* No GIF image opened? */
    if ((nullptr == m_gifLoader) ||
        (false == (*m_gifLoader)))
    {
        isSuccessful = false;
    }
    /* Finished? */
    else if (true == m_isFinished)
    {
        /* Redraw last scene. */
        gfx.drawBitmap(x, y, m_bitmap);
    }
    /* Delay? */
    else if ((true == m_timer.isTimerRunning()) &&
             (false == m_timer.isTimeout()))
    {
        /* Redraw last scene. */
        gfx.drawBitmap(x, y, m_bitmap);
    }
    else
    {
        bool    isImageShown    = false;
        BlockId blockId         = BLOCK_ID_EXTENSION;

        /* Walk through all blocks in the GIF image. */
        while((BLOCK_ID_TRAILER != blockId) && (false == isImageShown) && (true == isSuccessful) && (false == m_isTrailerFound))
        {
            /* Read block id. */
            if (false == m_gifLoader->read(&blockId, sizeof(blockId)))
            {
                isSuccessful = false;
            }
            /* Is the block a extension? */
            else if (BLOCK_ID_EXTENSION == blockId)
            {
                isSuccessful = parseExtension();
            }
            /* Is the block an image descriptor? */
            else if (BLOCK_ID_IMAGE_DESCRIPTOR == blockId)
            {
                isSuccessful = parseImageDescriptor();

                if (true == isSuccessful)
                {
                    /* Draw new scene. */
                    gfx.drawBitmap(x, y, m_bitmap);

                    /* Animation? */
                    if (true == m_isAnimation)
                    {
                        m_timer.start(m_delay);
                        isImageShown = true;
                    }
                }
            }
            /* Is it the end of the image? */
            else if (BLOCK_ID_TRAILER == blockId)
            {
                GIF_IMG_PLAYER_LOG_DEBUG("Trailer\n");
                GIF_IMG_PLAYER_RESET_FRAME_COUNT();

                /* Notify about the trailer. */
                m_isTrailerFound = true;

                /* Redraw last scene. */
                gfx.drawBitmap(x, y, m_bitmap);

                /* Animation running? */
                if (true == m_isAnimation)
                {
                    /* Is animation limited to a specific number of repeats? */
                    if (0U < m_loopCount)
                    {
                        --m_loopCount;

                        /* Animation finished? */
                        if (0U == m_loopCount)
                        {
                            m_isFinished = true;
                            m_timer.stop();
                        }
                    }
                    /* Infinite animation. */
                    else
                    {
                        ;
                    }

                    if (false == m_isFinished)
                    {
                        /* Restart from begin. */
                        if (false == m_gifLoader->seek(m_restartFilePos, SeekSet))
                        {
                            isSuccessful = false;
                        }
                        else
                        {
                            /* Force to continue until a scene is shown. */
                            blockId = BLOCK_ID_EXTENSION;
                        }
                    }
                }
                else
                {
                    m_isFinished = true;
                }
            }
            /* Unknown block id? */
            else
            {
                isSuccessful = false;
            }
        }

        /* Clean-up required because of any error? */
        if (false == isSuccessful)
        {
            if (nullptr != m_localColorTable)
            {
                delete[] m_localColorTable;
                m_localColorTable       = nullptr;
                m_localColorTableLength = 0U;
            }

            close();
        }
    }

    return isSuccessful;
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

void GifImgPlayer::cleanup()
{
    if (nullptr != m_gifLoader)
    {
        m_gifLoader->close();

        delete m_gifLoader;
        m_gifLoader = nullptr;
    }

    m_bitmap.release();

    if (nullptr != m_imageDataBlock)
    {
        delete[] m_imageDataBlock;
        m_imageDataBlock = nullptr;
    }

    if (nullptr != m_globalColorTable)
    {
        delete[] m_globalColorTable;
        
        m_globalColorTable          = nullptr;
        m_globalColorTableLength    = 0U;
    }

    if (nullptr != m_localColorTable)
    {
        delete[] m_localColorTable;
        
        m_localColorTable       = nullptr;
        m_localColorTableLength = 0U;
    }
}

bool GifImgPlayer::isFileSupported(const GifFileHeader& header) const
{
    bool        isSupported     = true;
    uint8_t     index;

    for(index = 0U; index < sizeof(header.signature); ++index)
    {
        if (GIF_SIGNATURE[index] != header.signature[index])
        {
            isSupported = false;
            break;
        }
    }

    if (true == isSupported)
    {
        bool isVersion89ASupported = true;
        bool isVersion87ASupported = true;

        for(index = 0U; index < sizeof(header.version); ++index)
        {
            if (GIF_VERSION_89A[index] != header.version[index])
            {
                isVersion89ASupported = false;
            }

            if (GIF_VERSION_87A[index] != header.version[index])
            {
                isVersion87ASupported = false;
            }
        }

        if ((false == isVersion89ASupported) && (false == isVersion87ASupported))
        {
            isSupported = false;
        }
    }

    return isSupported;
}

size_t GifImgPlayer::calcColorTableSize(uint8_t sizeExp) const
{
    uint8_t index   = 0U;
    size_t  size    = 1U;

    /* Calculation:
     * Size in byte = 3 * 2^(N + 1) 
     */

    /* Determine 2^(N + 1) */
    while(sizeExp >= index)
    {
        size *= 2U;

        ++index;
    }

    /* Consider RGB values */
    size *= 3U;

    return size;
}

bool GifImgPlayer::parseExtension()
{
    bool            isSuccessful = true;
    ExtensionLabel  label;

    /* Load extension label. */
    if (false == m_gifLoader->read(&label, sizeof(label)))
    {
        isSuccessful = false;
    }
    else
    {
        if (LABEL_GRAPHIC_CONTROL_EXTENSION == label)
        {
            isSuccessful = parseGraphicControlExentsion();
        }
        else if (LABEL_APPLICATION_EXTENSION == label)
        {
            isSuccessful = parseApplicationExtension();
        }
        /* Skip every other extension. */
        else
        {
            GIF_IMG_PLAYER_LOG_DEBUG("Extension\n");
            GIF_IMG_PLAYER_LOG_DEBUG("\tSkip unknown label: %u\n", label);

            if (false == skipBlock())
            {
                isSuccessful = false;
            }
        }
    }

    return isSuccessful;
}

bool GifImgPlayer::parseImageDescriptor()
{
    bool            isSuccessful = true;
    ImageDescriptor imageDescriptor;

    if (false == m_gifLoader->read(&imageDescriptor, sizeof(imageDescriptor)))
    {
        isSuccessful = false;
    }
    else
    {
        GIF_IMG_PLAYER_INC_FRAME_COUNT();
        GIF_IMG_PLAYER_LOG_DEBUG("Image descriptor (%u)\n", gFrameCnt);
        GIF_IMG_PLAYER_LOG_DEBUG("\tImage left  : %u\n", imageDescriptor.imageLeft);
        GIF_IMG_PLAYER_LOG_DEBUG("\tImage top   : %u\n", imageDescriptor.imageTop);
        GIF_IMG_PLAYER_LOG_DEBUG("\tImage width : %u\n", imageDescriptor.imageWidth);
        GIF_IMG_PLAYER_LOG_DEBUG("\tImage height: %u\n", imageDescriptor.imageHeight);
        GIF_IMG_PLAYER_LOG_DEBUG("\tPacked field\n");
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tLocal color table size exponent: %u\n", imageDescriptor.packedField.localColorTableSizeExp);
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tSort flag                      : %u\n", imageDescriptor.packedField.sortFlag);
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tInterlace flag                 : %u\n", imageDescriptor.packedField.interlaceFlag);
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tLocal color table flag         : %u\n", imageDescriptor.packedField.localColorTableFlag);
        GIF_IMG_PLAYER_LOG_DEBUG("\tLocal color table size: %u colors\n", calcColorTableSize(imageDescriptor.packedField.localColorTableSizeExp) / 3U);

        /* The image descriptor specifies the image left position and image
         * top position of where the image should begin on the canvas.
         */
        m_canvas.setOffsetX(imageDescriptor.imageLeft);
        m_canvas.setOffsetY(imageDescriptor.imageTop);
        m_canvas.setWidth(imageDescriptor.imageWidth);
        m_canvas.setHeight(imageDescriptor.imageHeight);

        /* Destroy any old local color table. */
        if (nullptr != m_localColorTable)
        {
            delete[] m_localColorTable;
            m_localColorTable       = nullptr;
            m_localColorTableLength = 0U;
        }
        
        /* Local color table available? */
        if (0U != imageDescriptor.packedField.localColorTableFlag)
        {
            size_t localColorTableSize = calcColorTableSize(imageDescriptor.packedField.localColorTableSizeExp);

            m_localColorTableLength = localColorTableSize / sizeof(PaletteColor);
            m_localColorTable       = new(std::nothrow) PaletteColor[m_localColorTableLength];

            if (nullptr == m_localColorTable)
            {
                m_localColorTableLength = 0U;

                isSuccessful = false;
            }
            else if (false == m_gifLoader->read(m_localColorTable, localColorTableSize))
            {
                isSuccessful = false;
            }
            else
            {
                ;
            }
        }

        /* Apply disposal method after the canvas is set for the related area. */
        applyDisposalMethod();

        /* Process image data */
        if (true == isSuccessful)
        {
            uint8_t lzwMinCodeSize = 0U;

            if (false == m_gifLoader->read(&lzwMinCodeSize, sizeof(lzwMinCodeSize)))
            {
                isSuccessful = false;
            }
            else
            {
                LzwDecoder                      lzwDecoder;
                LzwDecoder::ReadFromInStream    readFromCodeStreamFunc  =
                    [this](uint8_t& data) -> bool
                    {
                        return this->readFromCodeStream(data);
                    };
                LzwDecoder::WriteToOutStream    writeToIndexStreamFunc  =
                    [this](uint8_t data) -> bool
                    {
                        return this->writeToIndexStream(data);
                    };
                uint8_t                         blockTerminator         = 0U;

                /* Reset data block before start decoding the next block. */
                m_imageDataBlockIdx = 0U;
                m_imageDataBlockLength = 0U;

                /* Reset coordinates used for drawing. */
                m_posX = 0U;
                m_posY = 0U;

                lzwDecoder.init(lzwMinCodeSize);

                if (false == lzwDecoder.decode(readFromCodeStreamFunc, writeToIndexStreamFunc))
                {
                    isSuccessful = false;
                }

                lzwDecoder.deInit();

                /* After the image data, the block terminator marks the end. */
                if (false == m_gifLoader->read(&blockTerminator, sizeof(blockTerminator)))
                {
                    isSuccessful = false;
                }
                else if (0U != blockTerminator)
                {
                    isSuccessful = false;
                }
                else
                {
                    ;
                }
            }
        }
    }

    return isSuccessful;
}

void GifImgPlayer::applyDisposalMethod()
{
    switch(m_disposalMethod)
    {
    /* GIF 89a specification: No disposal specified. The decoder is not required to take any action. */
    case DISPOSAL_METHOD_NO_ACTION:
        break;
    
    /* GIF 89a specification: Do not dispose. The graphic is to be left in place. */
    case DISPOSAL_METHOD_NO_DISPOSE:
        break;
    
    /* GIF 89a specification: Restore to background color. The area used by the graphic must be restored to the background color. */
    case DISPOSAL_METHOD_RESTORE_TO_BACKGROUND:
        /* If no global color table is available, the background color index is invalid and the background will be treated as transparent. */
        if (nullptr == m_globalColorTable)
        {
            m_canvas.fillScreen(ColorDef::BLACK);
        }
        /* If global color table is available, but transparency flag is enabled, treat the background color index as transparent color index. */
        else if (true == m_isTransparencyEnabled)
        {
            m_canvas.fillScreen(ColorDef::BLACK);
        }
        /* Restore to background color. Only valid because global color table is available. */
        else
        {
            PaletteColor*   paletteColor = &m_globalColorTable[m_bgColorIndex];
            Color           bgColor(paletteColor->red, paletteColor->green, paletteColor->blue);

            m_canvas.fillScreen(bgColor);
        }
        break;
    
    /* GIF 89a specification: Restore to previous. The decoder is required to restore the area overwritten by the graphic with what was there prior to rendering the graphic. */
    case DISPOSAL_METHOD_RESTORE_TO_PREVIOUS:
        {
            PaletteColor*   paletteColor = &m_globalColorTable[m_bgColorIndex];
            Color           bgColor(paletteColor->red, paletteColor->green, paletteColor->blue);

            /* GIF 89a specification:
             * The mode Restore To Previous is intended to be used in small sections of the graphic; the use of this mode imposes
             * severe demands on the decoder to store the section of the graphic that needs to be saved. For this reason, this mode should be used
             * sparingly.  This mode is not intended to save an entire graphic or large areas of a graphic; when this is the case, the encoder should
             * make every attempt to make the sections of the graphic to be restored be separate graphics in the data stream. In the case where
             * a decoder is not capable of saving an area of a graphic marked as Restore To Previous, it is recommended that a decoder restore to
             * the background color.
             */
            m_canvas.fillScreen(bgColor);
        }
        break;

    default:
        /* Not defined by GIF 89a specification. */
        break;
    }
}

bool GifImgPlayer::parseGraphicControlExentsion()
{
    bool                    isSuccessful    = true;
    uint8_t                 blockSize       = 0x0U;
    uint8_t                 blockTerminator = 0x0U;
    GraphicControlExtension gce;

    if (false == m_gifLoader->read(&blockSize, sizeof(blockSize)))
    {
        isSuccessful = false;
    }
    else if (sizeof(GraphicControlExtension) != blockSize)
    {
        isSuccessful = false;
    }
    else if (false == m_gifLoader->read(&gce, sizeof(gce)))
    {
        isSuccessful = false;
    }
    else if (false == m_gifLoader->read(&blockTerminator, sizeof(blockTerminator)))
    {
        isSuccessful = false;
    }
    else if (0U != blockTerminator)
    {
        isSuccessful = false;
    }
    else
    {
        GIF_IMG_PLAYER_LOG_DEBUG("Graphic control extension\n");
        GIF_IMG_PLAYER_LOG_DEBUG("\tPacked field\n");
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tTransparent color flag: %u\n", gce.packedField.transparentColorFlag);
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tUser input flag       : %u\n", gce.packedField.userInputFlag);
        GIF_IMG_PLAYER_LOG_DEBUG("\t\tDisposal method       : %u - %s\n", gce.packedField.disposalMethod, getDisposalMethodAsString(gce.packedField.disposalMethod));
        GIF_IMG_PLAYER_LOG_DEBUG("\tDelay time             : %u * 1/100 s\n", gce.delayTime);
        GIF_IMG_PLAYER_LOG_DEBUG("\tTransparent color index: %u\n", gce.transparentColorIndex);

        if (0U == gce.packedField.transparentColorFlag)
        {
            GIF_IMG_PLAYER_LOG_DEBUG("\tTransparent color      : -\n");
        }
        else
        {
            if (nullptr == m_globalColorTable)
            {
                GIF_IMG_PLAYER_LOG_DEBUG("\tTransparent color      : error - no global color table available\n");
            }
            else
            {
                GIF_IMG_PLAYER_LOG_DEBUG("\tTransparent color      : 0x%02X%02X%02X\n",
                    m_globalColorTable[gce.transparentColorIndex].red,
                    m_globalColorTable[gce.transparentColorIndex].green,
                    m_globalColorTable[gce.transparentColorIndex].blue);
            }
        }

        m_delay                 = gce.delayTime * 10U; /* 0.1 ms --> ms */
        m_transparentColorIndex = gce.transparentColorIndex;

        if (0U == gce.packedField.transparentColorFlag)
        {
            m_isTransparencyEnabled = false;
        }
        else
        {
            m_isTransparencyEnabled = true;
        }

        /* The disposal method will be applied during next image descriptor handling. */
        m_disposalMethod = static_cast<DisposalMethod>(gce.packedField.disposalMethod);
    }

    return isSuccessful;
}

bool GifImgPlayer::parseApplicationExtension()
{
    bool                    isSuccessful    = true;
    uint8_t                 blockSize       = 0x0U;
    ApplicationExtension    appExt;

    /* Read application extension block size. */
    if (false == m_gifLoader->read(&blockSize, sizeof(blockSize)))
    {
        isSuccessful = false;
    }
    /*  Invalid? */
    else if (sizeof(appExt) != blockSize)
    {
        isSuccessful = false;
    }
    /* Read application extension. */
    else if (false == m_gifLoader->read(&appExt, sizeof(appExt)))
    {
        isSuccessful = false;
    }
    else
    {
        const void* vAppIdentifier  = appExt.identifier;
        const char* appIdentifier   = static_cast<const char*>(vAppIdentifier);
        const void* vAppAuthCode    = appExt.authenticationCode;
        const char* appAuthCode     = static_cast<const char*>(vAppAuthCode);

        /* Only the NETSCAPE 2.0 application is supported for animatins. */
        if ((0 == strncmp(appIdentifier, "NETSCAPE", sizeof(appExt.identifier))) &&
            (0 == strncmp(appAuthCode, "2.0", sizeof(appExt.authenticationCode))))
        {
            isSuccessful = parseNetscape20subBlocks();
        }
        else
        {
            GIF_IMG_PLAYER_LOG_DEBUG("Application extension\n");
            GIF_IMG_PLAYER_LOG_DEBUG("\tSkip unknown\n");

            /* Skip all sub-blocks, which are application specific. */
            isSuccessful = skipBlock();
        }
    }

    return isSuccessful;
}

bool GifImgPlayer::parseNetscape20subBlocks()
{
    bool    isSuccessful    = true;
    uint8_t subBlockSize    = 0xFFU;
    uint8_t subBlockId      = 0U;
    uint8_t blockTerminator = 0U;

    /* Read sub-block size. */
    if (false == m_gifLoader->read(&subBlockSize, sizeof(subBlockSize)))
    {
        isSuccessful = false;
    }
    /* The sub-block size shall contain the id and the loop counter. */
    else if (0x03U != subBlockSize)
    {
        isSuccessful = false;
    }
    /* Read the id. */
    else if (false == m_gifLoader->read(&subBlockId, sizeof(subBlockId)))
    {
        isSuccessful = false;
    }
    /* The id shall be 0x01. */
    else if (0x01U != subBlockId)
    {
        isSuccessful = false;
    }
    /* Read the loop counter. */
    else if (false == m_gifLoader->read(&m_loopCount, sizeof(m_loopCount)))
    {
        isSuccessful = false;
    }
    /* Read block terminator. */
    else if (false == m_gifLoader->read(&blockTerminator, sizeof(blockTerminator)))
    {
        isSuccessful = false;
    }
    /* Verify block terminator. */
    else if (0x00U != blockTerminator)
    {
        isSuccessful = false;
    }
    else
    {
        GIF_IMG_PLAYER_LOG_DEBUG("Application extension\n");
        GIF_IMG_PLAYER_LOG_DEBUG("\tNETSCAPE 2.0\n");
        GIF_IMG_PLAYER_LOG_DEBUG("\tLoop counter: %u\n", m_loopCount);

        m_isAnimation = true;

        /* Store position after application extension to know where to restart
         * the animation.
         */
        m_restartFilePos = m_gifLoader->position();
    }

    return isSuccessful;
}

bool GifImgPlayer::skipBlock()
{
    bool    isSuccessful    = true;
    uint8_t blockSize       = 0xFFU;

    while((0U < blockSize) && (true == isSuccessful))
    {
        if (false == m_gifLoader->read(&blockSize, sizeof(blockSize)))
        {
            isSuccessful = false;
        }
        else if (0U < blockSize)
        {
            if (false == m_gifLoader->seek(blockSize, SeekCur))
            {
                isSuccessful = false;
            }
        }
        else
        {
            ;
        }
    }

    return isSuccessful;
}

size_t GifImgPlayer::loadImageDataBlock(uint8_t* block, size_t size)
{
    bool        isSuccessful    = true;
    uint8_t     blockSize       = 0U;

    if ((nullptr == block) ||
        (0U == size))
    {
        isSuccessful = false;
    }
    else if (false == m_gifLoader->read(&blockSize, sizeof(blockSize)))
    {
        isSuccessful = false;
    }
    else if (0U < blockSize)
    {
        if (size < blockSize)
        {
            isSuccessful = false;
        }
        else if (false == m_gifLoader->read(block, blockSize))
        {
            isSuccessful = false;
        }
        else
        {
            ;
        }
    }

    if (false == isSuccessful)
    {
        blockSize = 0U;
    }

    return blockSize;
}

bool GifImgPlayer::readFromCodeStream(uint8_t& data)
{
    bool isSuccessful = true;

    if ((0U == m_imageDataBlockLength) ||
        (m_imageDataBlockLength <= m_imageDataBlockIdx))
    {
        m_imageDataBlockLength = loadImageDataBlock(m_imageDataBlock, IMAGE_DATA_BLOCK_SIZE);

        if (0U == m_imageDataBlockLength)
        {
            isSuccessful = false;
        }
        else
        {
            m_imageDataBlockIdx = 0U;
        }
    }

    if (m_imageDataBlockLength > m_imageDataBlockIdx)
    {
        data = m_imageDataBlock[m_imageDataBlockIdx];
        ++m_imageDataBlockIdx;
    }

    return isSuccessful;
}

bool GifImgPlayer::writeToIndexStream(uint8_t data)
{
    bool            isSuccessful        = false;
    PaletteColor*   colorTable          = (nullptr != m_localColorTable) ? m_localColorTable : m_globalColorTable;
    size_t          colorTableLength    = (nullptr != m_localColorTable) ? m_localColorTableLength : m_globalColorTableLength;

    /* Color table must be available and
     * the color index must be part of it.
     */
    if ((nullptr != colorTable) &&
        (colorTableLength > data))
    {
        /* If transparency is not enabled or
         * it is enabled and the color index is not transparent,
         * the pixel will be drawn.
         */
        if ((false == m_isTransparencyEnabled) ||
            ((true == m_isTransparencyEnabled) && (m_transparentColorIndex != data)))
        {
            PaletteColor*   paletteColor = &colorTable[data];
            Color           color(paletteColor->red, paletteColor->green, paletteColor->blue);

            m_canvas.drawPixel(m_posX, m_posY, color);
        }

        ++m_posX;
        if (m_canvas.getWidth() <= m_posX)
        {
            m_posX = 0;
            ++m_posY;
        }

        isSuccessful = true;
    }

    return isSuccessful;
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/

#if GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE

/**
 * Get user friendly description of the disposal method.
 * 
 * @param[in] disposalMethod    The disposal method.
 * 
 * @return Description as string
 */
static const char* getDisposalMethodAsString(uint8_t disposalMethod)
{
    const char* disposalMethodStr = "To be defined.";

    switch(disposalMethod)
    {
    case 0U:
        disposalMethodStr = "No disposal specified.";
        break;
    
    case 1U:
        disposalMethodStr = "Do not dispose. Leave graphic in place.";
        break;

    case 2U:
        disposalMethodStr = "Restore to background color.";
        break;

    case 3U:
        disposalMethodStr = "Restore to previous.";
        break;

    default:
        break;
    }

    return disposalMethodStr;
}

#endif  /* GIF_IMG_PLAYER_DEBUG_MODE == GIF_IMG_PLAYER_DEBUG_ENABLE */