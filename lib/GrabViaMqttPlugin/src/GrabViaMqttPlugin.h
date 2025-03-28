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
 * @brief  Grab information via MQTT API plugin
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup PLUGIN
 *
 * @{
 */

#ifndef GRAB_VIA_MQTT_PLUGIN_H
#define GRAB_VIA_MQTT_PLUGIN_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "./internal/View.h"

#include <stdint.h>
#include <PluginWithConfig.hpp>
#include <Mutex.hpp>
#include <FileSystem.h>
#include <FileMgrService.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Grab information from a MQTT broker and display it.
 */
class GrabViaMqttPlugin : public PluginWithConfig
{
public:

    /**
     * Constructs the plugin.
     *
     * @param[in] name  Plugin name (must exist over lifetime)
     * @param[in] uid   Unique id
     */
    GrabViaMqttPlugin(const char* name, uint16_t uid) :
        PluginWithConfig(name, uid, FILESYSTEM),
        m_view(),
        m_path(),
        m_filter(1024U),
        m_iconFileId(FileMgrService::FILE_ID_INVALID),
        m_format("%s"),
        m_delimiter("::"),
        m_multiplier(1.0f),
        m_offset(0.0f),
        m_mutex(),
        m_hasTopicChanged(false)
    {
        (void)m_mutex.create();
    }

    /**
     * Destroys the plugin.
     */
    ~GrabViaMqttPlugin()
    {
        m_mutex.destroy();
    }

    /**
     * Plugin creation method, used to register on the plugin manager.
     *
     * @param[in] name  Plugin name (must exist over lifetime)
     * @param[in] uid   Unique id
     *
     * @return If successful, it will return the pointer to the plugin instance, otherwise nullptr.
     */
    static IPluginMaintenance* create(const char* name, uint16_t uid)
    {
        return new (std::nothrow) GrabViaMqttPlugin(name, uid);
    }

    /**
     * Get font type.
     *
     * @return The font type the plugin uses.
     */
    Fonts::FontType getFontType() const final
    {
        return m_view.getFontType();
    }

    /**
     * Set font type.
     * The plugin may skip the font type in case it gets conflicts with the layout.
     *
     * A font type change will only be considered if it is set before the start()
     * method is called!
     *
     * @param[in] fontType  The font type which the plugin shall use.
     */
    void setFontType(Fonts::FontType fontType) final
    {
        m_view.setFontType(fontType);
    }

    /**
     * Get plugin topics, which can be get/set via different communication
     * interfaces like REST, websocket, MQTT, etc.
     *
     * Example:
     * <code>{.json}
     * {
     *     "topics": [
     *         "text"
     *     ]
     * }
     * </code>
     *
     * By default a topic is readable and writeable.
     * This can be set explicit with the "access" key with the following possible
     * values:
     * - Only readable: "r"
     * - Only writeable: "w"
     * - Readable and writeable: "rw"
     *
     * Example:
     * <code>{.json}
     * {
     *     "topics": [{
     *         "name": "text",
     *         "access": "r"
     *     }]
     * }
     * </code>
     *
     * Home Assistant MQTT discovery support can be added with the "ha" JSON object inside
     * the "extra" JSON object. The Home Assistant extension supports only loading by file.
     * <code>{.json}
     * {
     *     "topics": [{
     *         "name": "text",
     *         "extra": {
     *             "ha": "myHomeAssistantConfig.json"
     *         }
     *     }]
     * }
     * </code>
     *
     * Extra information can be loaded from a file too. This is useful for complex
     * configurations and to keep program memory usage low.
     * <code>{.json}
     * {
     *     "topics": [{
     *         "name": "text",
     *         "extra": "extra.json"
     *    }]
     * }
     * </code>
     *
     * @param[out] topics   Topis in JSON format
     */
    void getTopics(JsonArray& topics) const final;

    /**
     * Get a topic data.
     * Note, currently only JSON format is supported.
     *
     * @param[in]   topic   The topic which data shall be retrieved.
     * @param[out]  value   The topic value in JSON format.
     *
     * @return If successful it will return true otherwise false.
     */
    bool getTopic(const String& topic, JsonObject& value) const final;

    /**
     * Set a topic data.
     * Note, currently only JSON format is supported.
     *
     * @param[in]   topic   The topic which data shall be retrieved.
     * @param[in]   value   The topic value in JSON format.
     *
     * @return If successful it will return true otherwise false.
     */
    bool setTopic(const String& topic, const JsonObjectConst& value) final;

    /**
     * Is the topic content changed since last time?
     * Every readable volatile topic shall support this. Otherwise the topic
     * handlers might not be able to provide updated information.
     *
     * @param[in] topic The topic which to check.
     *
     * @return If the topic content changed since last time, it will return true otherwise false.
     */
    bool hasTopicChanged(const String& topic) final;

    /**
     * Start the plugin. This is called only once during plugin lifetime.
     * It can be used as deferred initialization (after the constructor)
     * and provides the canvas size.
     *
     * If your display layout depends on canvas or font size, calculate it
     * here.
     *
     * Overwrite it if your plugin needs to know that it was installed.
     *
     * @param[in] width     Display width in pixel
     * @param[in] height    Display height in pixel
     */
    void start(uint16_t width, uint16_t height) final;

    /**
     * Stop the plugin. This is called only once during plugin lifetime.
     * It can be used as a first clean-up, before the plugin will be destroyed.
     *
     * Overwrite it if your plugin needs to know that it will be uninstalled.
     */
    void stop() final;

    /**
     * Process the plugin.
     * Overwrite it if your plugin has cyclic stuff to do without being in a
     * active slot.
     *
     * @param[in] isConnected   The network connection status. If network
     *                          connection is established, it will be true otherwise false.
     */
    void process(bool isConnected) final;

    /**
     * Update the display.
     * The scheduler will call this method periodically.
     *
     * @param[in] gfx   Display graphics interface
     */
    void update(YAGfx& gfx) final;

private:

    /**
     * Plugin topic, used to read/write the configuration.
     */
    static const char*       TOPIC_CONFIG;

    _GrabViaMqttPlugin::View m_view;            /**< View with all widgets. */
    String                   m_path;            /**< MQTT topic path */
    DynamicJsonDocument      m_filter;          /**< Filter used for the response in JSON format. */
    FileMgrService::FileId   m_iconFileId;      /**< Icon file id. */
    String                   m_format;          /**< Format used to embed the retrieved filtered value. */
    String                   m_delimiter;       /**< Delimiter is used in case several values shall be shown, because of an JSON array. */
    float                    m_multiplier;      /**< If grabbed value is a number, it will be multiplied with the multiplier. */
    float                    m_offset;          /**< If grabbed value is a number, the offset will be added after the multiplication with the multiplier. */
    mutable MutexRecursive   m_mutex;           /**< Mutex to protect against concurrent access. */
    bool                     m_hasTopicChanged; /**< Has the topic content changed? */

    /**
     * Get configuration in JSON.
     *
     * @param[out] cfg  Configuration
     */
    void getConfiguration(JsonObject& jsonCfg) const final;

    /**
     * Set configuration in JSON.
     *
     * @param[in] cfg   Configuration
     *
     * @return If successful set, it will return true otherwise false.
     */
    bool setConfiguration(const JsonObjectConst& jsonCfg) final;

    /**
     * Request new data.
     *
     * @return If successful it will return true otherwise false.
     */
    bool startHttpRequest(void);

    /**
     * Get value from JSON source by the filter.
     * It will be called recursively and can handle nested JSON objects and arrays.
     *
     * @param[in]   src     Source in JSON format
     * @param[in]   filter  Filter in JSON format
     * @param[out]  values  Array of values in JSON format
     */
    void getJsonValueByFilter(JsonVariantConst src, JsonVariantConst filter, JsonArray& values);

    /**
     * Clear the task proxy queue.
     */
    void clearQueue();

    /**
     * Subscribe MQTT topic to be informed about value changes.
     */
    void subscribe();

    /**
     * Unsubscribe MQTT topic to stop on change notifications.
     */
    void unsubscribe();

    /**
     * The MQTT callback is registered by subscription and will be called on change by
     * the MQTT service.
     *
     * @param[in] topic     Topic
     * @param[in] payload   Topic payload
     * @param[in] size      Topic payload size in byte
     */
    void mqttTopicCallback(const String& topic, const uint8_t* payload, size_t size);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* GRAB_VIA_MQTT_PLUGIN_H */

/** @} */
