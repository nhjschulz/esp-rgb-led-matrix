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
 * @brief  Plugin manager
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "PluginMgr.h"
#include "DisplayMgr.h"
#include "FileSystem.h"
#include "Plugin.hpp"
#include "JsonFile.h"

#include <Logging.h>
#include <ArduinoJson.h>
#include <Util.h>
#include <SettingsService.h>
#include <TopicHandlerService.h>

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

/* Initialize static members */
const char* PluginMgr::CONFIG_FILE_NAME        = "slotConfig.json";
const char* PluginMgr::MQTT_SPECIAL_CHARACTERS = "+#*>$"; /* See MQTT specification */

/******************************************************************************
 * Public Methods
 *****************************************************************************/

void PluginMgr::begin()
{
    SettingsService& settings = SettingsService::getInstance();

    if (false == settings.open(true))
    {
        m_deviceId = settings.getHostname().getDefault();
    }
    else
    {
        m_deviceId = settings.getHostname().getValue();

        settings.close();
    }

    createPluginConfigDirectory();
}

IPluginMaintenance* PluginMgr::install(const char* name, uint8_t slotId)
{
    IPluginMaintenance* plugin = m_pluginFactory.createPlugin(name);

    if (nullptr != plugin)
    {
        if (false == install(plugin, slotId))
        {
            m_pluginFactory.destroyPlugin(plugin);
            plugin = nullptr;
        }
    }

    return plugin;
}

bool PluginMgr::uninstall(IPluginMaintenance* plugin)
{
    bool status = false;

    if (nullptr != plugin)
    {
        status = DisplayMgr::getInstance().uninstallPlugin(plugin);

        if (true == status)
        {
            unregisterTopicsByUID(m_deviceId, plugin, true);
            unregisterTopicsByAlias(m_deviceId, plugin, true);

            m_pluginFactory.destroyPlugin(plugin);
        }
    }

    return status;
}

bool PluginMgr::setPluginAliasName(IPluginMaintenance* plugin, const String& alias)
{
    bool isSuccessful = false;

    if ((nullptr != plugin) &&
        (plugin->getAlias() != alias) &&
        (true == isPluginAliasValid(alias)))
    {
        /* First remove current registered topics. */
        unregisterTopicsByAlias(m_deviceId, plugin, true);

        /* Set new alias */
        plugin->setAlias(alias);

        /* Register web API, based on new alias. */
        registerTopicsByAlias(m_deviceId, plugin);

        isSuccessful = true;
    }

    return isSuccessful;
}

void PluginMgr::unregisterAllPluginTopics()
{
    uint8_t maxSlots = DisplayMgr::getInstance().getMaxSlots();
    uint8_t slotId   = 0U;

    for (slotId = 0U; slotId < maxSlots; ++slotId)
    {
        IPluginMaintenance* plugin = DisplayMgr::getInstance().getPluginInSlot(slotId);

        unregisterTopicsByUID(m_deviceId, plugin, false);
        unregisterTopicsByAlias(m_deviceId, plugin, false);
    }
}

bool PluginMgr::load()
{
    bool                isSuccessful = true;
    JsonFile            jsonFile(FILESYSTEM);
    const size_t        JSON_DOC_SIZE = 4096U;
    DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
    String              fullConfigFileName  = Plugin::CONFIG_PATH;

    fullConfigFileName                     += "/";
    fullConfigFileName                     += CONFIG_FILE_NAME;

    if (false == jsonFile.load(fullConfigFileName, jsonDoc))
    {
        LOG_WARNING("Failed to load file %s.", fullConfigFileName.c_str());
        isSuccessful = false;
    }
    else if (true == jsonDoc.overflowed())
    {
        LOG_ERROR("JSON document has less memory available.");
        isSuccessful = false;
    }
    else
    {
        JsonArray     jsonSlots = jsonDoc["slotConfiguration"].as<JsonArray>();
        uint8_t       slotId    = 0;
        const uint8_t MAX_SLOTS = DisplayMgr::getInstance().getMaxSlots();

        for (JsonObject jsonSlot : jsonSlots)
        {
            prepareSlotByConfiguration(slotId, jsonSlot);

            ++slotId;
            if (MAX_SLOTS <= slotId)
            {
                break;
            }
        }
    }

    return isSuccessful;
}

void PluginMgr::save()
{
    String              installation;
    uint8_t             slotId        = 0;
    const size_t        JSON_DOC_SIZE = 4096U;
    DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
    JsonArray           jsonSlots = jsonDoc.createNestedArray("slotConfiguration");
    JsonFile            jsonFile(FILESYSTEM);
    String              fullConfigFileName  = Plugin::CONFIG_PATH;
    DisplayMgr&         displayMgr          = DisplayMgr::getInstance();
    uint8_t             stickySlotId        = displayMgr.getStickySlot();

    fullConfigFileName                     += "/";
    fullConfigFileName                     += CONFIG_FILE_NAME;

    for (slotId = 0; slotId < displayMgr.getMaxSlots(); ++slotId)
    {
        IPluginMaintenance* plugin   = displayMgr.getPluginInSlot(slotId);
        JsonObject          jsonSlot = jsonSlots.createNestedObject();

        if (nullptr == plugin)
        {
            jsonSlot["name"]     = "";
            jsonSlot["uid"]      = 0;
            jsonSlot["alias"]    = "";
            jsonSlot["fontType"] = Fonts::fontTypeToStr(Fonts::FONT_TYPE_DEFAULT);
        }
        else
        {
            jsonSlot["name"]     = plugin->getName();
            jsonSlot["uid"]      = plugin->getUID();
            jsonSlot["alias"]    = plugin->getAlias();
            jsonSlot["fontType"] = Fonts::fontTypeToStr(plugin->getFontType());
        }

        jsonSlot["duration"] = displayMgr.getSlotDuration(slotId);

        if (stickySlotId == slotId)
        {
            jsonSlot["isSticky"] = true;
        }
        else
        {
            jsonSlot["isSticky"] = false;
        }

        jsonSlot["isDisabled"] = displayMgr.isSlotDisabled(slotId);
    }

    if (true == jsonDoc.overflowed())
    {
        LOG_ERROR("JSON document has less memory available.");
    }
    else if (false == jsonFile.save(fullConfigFileName, jsonDoc))
    {
        LOG_ERROR("Couldn't save slot configuration.");
    }
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

void PluginMgr::createPluginConfigDirectory()
{
    if (false == FILESYSTEM.exists(Plugin::CONFIG_PATH))
    {
        if (false == FILESYSTEM.mkdir(Plugin::CONFIG_PATH))
        {
            LOG_WARNING("Couldn't create directory: %s", Plugin::CONFIG_PATH);
        }
    }
}

void PluginMgr::prepareSlotByConfiguration(uint8_t slotId, const JsonObject& jsonSlot)
{
    JsonVariantConst jsonName       = jsonSlot["name"];
    JsonVariantConst jsonUid        = jsonSlot["uid"];
    JsonVariantConst jsonAlias      = jsonSlot["alias"];
    JsonVariantConst jsonFontType   = jsonSlot["fontType"];
    JsonVariantConst jsonDuration   = jsonSlot["duration"];
    JsonVariantConst jsonIsSticky   = jsonSlot["isSticky"];
    JsonVariantConst jsonIsDisabled = jsonSlot["isDisabled"];

    if (false == jsonName.is<String>())
    {
        LOG_ERROR("Slot %u: Name is missing.", slotId);
    }
    else if (false == jsonUid.is<uint16_t>())
    {
        LOG_ERROR("Slot %u: UID is missing.", slotId);
    }
    else if (false == jsonAlias.is<String>())
    {
        LOG_ERROR("Slot %u: Alias is missing.", slotId);
    }
    else if (false == jsonFontType.is<String>())
    {
        LOG_ERROR("Slot %u: Font type is missing.", slotId);
    }
    else if (false == jsonDuration.is<uint32_t>())
    {
        LOG_ERROR("Slot %u: Slot duration is missing.", slotId);
    }
    else
    {
        DisplayMgr& displayMgr = DisplayMgr::getInstance();
        const char* name       = jsonName.as<const char*>();
        uint32_t    duration   = jsonDuration.as<uint32_t>();
        bool        isSticky   = false;
        bool        isDisabled = false;

        /* Optional */
        if (false == jsonIsSticky.is<bool>())
        {
            LOG_WARNING("Slot %u: Is sticky flag is missing.", slotId);
        }
        else
        {
            isSticky = jsonIsSticky.as<bool>();
        }

        if (false == jsonIsDisabled.is<bool>())
        {
            LOG_WARNING("Slot %u: Is disabled flag is missing.", slotId);
        }
        else
        {
            isDisabled = jsonIsDisabled.as<bool>();
        }

        /* Name available? */
        if ('\0' != name[0])
        {
            IPluginMaintenance* plugin = DisplayMgr::getInstance().getPluginInSlot(slotId);

            /* If there is already a plugin installed, it may be a system plugin.
             * In this case skip it, otherwise continue.
             */
            if (nullptr == plugin)
            {
                uint16_t uid = jsonUid.as<uint16_t>();

                plugin       = m_pluginFactory.createPlugin(name, uid);

                if (nullptr == plugin)
                {
                    LOG_ERROR("Couldn't create plugin %s (uid %u) in slot %u.", name, uid, slotId);
                }
                else
                {
                    String          alias         = jsonAlias.as<String>();
                    String          filteredAlias = filterPluginAlias(alias);
                    String          fontTypeStr   = jsonFontType.as<String>();
                    Fonts::FontType fontType      = Fonts::strToFontType(fontTypeStr.c_str());

                    plugin->setAlias(filteredAlias);
                    plugin->setFontType(fontType);

                    if (false == install(plugin, slotId))
                    {
                        LOG_WARNING("Couldn't install %s (uid %u) in slot %u.", name, uid, slotId);

                        m_pluginFactory.destroyPlugin(plugin);
                        plugin = nullptr;
                    }
                    else
                    {
                        plugin->enable();
                    }
                }
            }
        }

        displayMgr.setSlotDuration(slotId, duration);

        if (true == isSticky)
        {
            displayMgr.setSlotSticky(slotId);
        }

        if (false == isDisabled)
        {
            displayMgr.enableSlot(slotId);
        }
        else
        {
            displayMgr.disableSlot(slotId);
        }
    }
}

bool PluginMgr::install(IPluginMaintenance* plugin, uint8_t slotId)
{
    bool isSuccessful = false;

    if (nullptr != plugin)
    {
        if (SlotList::SLOT_ID_INVALID == slotId)
        {
            isSuccessful = installToAutoSlot(plugin);
        }
        else
        {
            isSuccessful = installToSlot(plugin, slotId);
        }

        if (true == isSuccessful)
        {
            registerTopicsByUID(m_deviceId, plugin);
            registerTopicsByAlias(m_deviceId, plugin);
        }
    }

    return isSuccessful;
}

bool PluginMgr::installToAutoSlot(IPluginMaintenance* plugin)
{
    bool status = false;

    if (nullptr != plugin)
    {
        if (SlotList::SLOT_ID_INVALID == DisplayMgr::getInstance().installPlugin(plugin))
        {
            LOG_ERROR("Couldn't install plugin %s.", plugin->getName());
        }
        else
        {
            status = true;
        }
    }

    return status;
}

bool PluginMgr::installToSlot(IPluginMaintenance* plugin, uint8_t slotId)
{
    bool status = false;

    if (nullptr != plugin)
    {
        if (SlotList::SLOT_ID_INVALID == DisplayMgr::getInstance().installPlugin(plugin, slotId))
        {
            LOG_ERROR("Couldn't install plugin %s to slot %u.", plugin->getName(), slotId);
        }
        else
        {
            status = true;
        }
    }

    return status;
}

bool PluginMgr::isPluginAliasValid(const String& alias)
{
    const size_t MQTT_SPECIAL_CHARACTERS_LEN = strlen(MQTT_SPECIAL_CHARACTERS);
    bool         isValid                     = true;
    size_t       idx                         = 0U;

    while ((MQTT_SPECIAL_CHARACTERS_LEN > idx) && (true == isValid))
    {
        if (0 <= alias.indexOf(MQTT_SPECIAL_CHARACTERS[idx]))
        {
            isValid = false;
        }
        else
        {
            ++idx;
        }
    }

    return isValid;
}

String PluginMgr::filterPluginAlias(const String& alias)
{
    const size_t MQTT_SPECIAL_CHARACTERS_LEN = strlen(MQTT_SPECIAL_CHARACTERS);
    size_t       idx                         = 0U;
    String       filteredPluginAlias         = alias;

    while (MQTT_SPECIAL_CHARACTERS_LEN > idx)
    {
        int pos = filteredPluginAlias.indexOf(MQTT_SPECIAL_CHARACTERS[idx]);

        while (0 <= pos)
        {
            filteredPluginAlias.remove(pos, 1U);
            pos = filteredPluginAlias.indexOf(MQTT_SPECIAL_CHARACTERS[idx]);
        }

        ++idx;
    }

    return filteredPluginAlias;
}

String PluginMgr::getEntityIdByPluginUid(uint16_t uid)
{
    return String("display/uid/") + uid;
}

String PluginMgr::getEntityIdByPluginAlias(const String& alias)
{
    return String("display/alias/") + alias;
}

void PluginMgr::registerTopicsByUID(const String& deviceId, IPluginMaintenance* plugin)
{
    if (nullptr != plugin)
    {
        String entityId = getEntityIdByPluginUid(plugin->getUID());

        TopicHandlerService::getInstance().registerTopics(m_deviceId, entityId.c_str(), plugin);
    }
}

void PluginMgr::unregisterTopicsByUID(const String& deviceId, IPluginMaintenance* plugin, bool purge)
{
    if (nullptr != plugin)
    {
        String entityId = getEntityIdByPluginUid(plugin->getUID());

        TopicHandlerService::getInstance().unregisterTopics(m_deviceId, entityId.c_str(), plugin, purge);
    }
}

void PluginMgr::registerTopicsByAlias(const String& deviceId, IPluginMaintenance* plugin)
{
    if ((nullptr != plugin) &&
        (false == plugin->getAlias().isEmpty()))
    {
        String entityId = getEntityIdByPluginAlias(plugin->getAlias());

        TopicHandlerService::getInstance().registerTopics(m_deviceId, entityId.c_str(), plugin);
    }
}

void PluginMgr::unregisterTopicsByAlias(const String& deviceId, IPluginMaintenance* plugin, bool purge)
{
    if ((nullptr != plugin) &&
        (false == plugin->getAlias().isEmpty()))
    {
        String entityId = getEntityIdByPluginAlias(plugin->getAlias());

        TopicHandlerService::getInstance().unregisterTopics(m_deviceId, entityId.c_str(), plugin, purge);
    }
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
