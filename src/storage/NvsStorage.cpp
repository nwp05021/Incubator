#include "storage/NvsStorage.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "NvsStorage";

namespace incubator::storage
{

bool NvsStorage::init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition erased, reinitializing");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return false;
    }
    return openRW();
}

bool NvsStorage::openRW()
{
    if (m_open) return true;
    esp_err_t err = nvs_open(kNamespace, NVS_READWRITE, &m_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }
    m_open = true;
    return true;
}

void NvsStorage::close()
{
    if (m_open) {
        nvs_close(m_handle);
        m_open = false;
    }
}

bool NvsStorage::saveBlob(const char* key, const void* data, size_t size)
{
    if (!m_open) return false;
    esp_err_t err = nvs_set_blob(m_handle, key, data, size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "saveBlob[%s] failed: %s", key, esp_err_to_name(err));
        return false;
    }
    err = nvs_commit(m_handle);
    return (err == ESP_OK);
}

bool NvsStorage::loadBlob(const char* key, void* data, size_t expectedSize)
{
    if (!m_open) return false;
    size_t size = expectedSize;
    esp_err_t err = nvs_get_blob(m_handle, key, data, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "loadBlob[%s] not found (first boot)", key);
        return false;
    }
    if (err == ESP_ERR_NVS_INVALID_LENGTH) {
        ESP_LOGW(TAG, "loadBlob[%s] invalid stored size, erasing stale value", key);
        eraseKey(key);
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "loadBlob[%s] failed: %s", key, esp_err_to_name(err));
        return false;
    }
    if (size != expectedSize) {
        ESP_LOGW(TAG, "loadBlob[%s] size mismatch: got %u, expected %u",
                 key, static_cast<unsigned>(size), static_cast<unsigned>(expectedSize));
        return false;
    }
    return true;
}

bool NvsStorage::saveU32(const char* key, uint32_t value)
{
    if (!m_open) return false;
    esp_err_t err = nvs_set_u32(m_handle, key, value);
    if (err != ESP_OK) return false;
    return nvs_commit(m_handle) == ESP_OK;
}

bool NvsStorage::loadU32(const char* key, uint32_t& outValue)
{
    if (!m_open) return false;
    esp_err_t err = nvs_get_u32(m_handle, key, &outValue);
    return (err == ESP_OK);
}

bool NvsStorage::eraseKey(const char* key)
{
    if (!m_open) return false;
    esp_err_t err = nvs_erase_key(m_handle, key);
    if (err == ESP_ERR_NVS_NOT_FOUND) return true;
    if (err != ESP_OK) return false;
    return nvs_commit(m_handle) == ESP_OK;
}

bool NvsStorage::eraseAll()
{
    if (!m_open) return false;
    esp_err_t err = nvs_erase_all(m_handle);
    if (err != ESP_OK) return false;
    return nvs_commit(m_handle) == ESP_OK;
}

} // namespace incubator::storage
