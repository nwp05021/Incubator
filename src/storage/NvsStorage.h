#pragma once
#include <nvs_flash.h>
#include <nvs.h>
#include <cstdint>
#include <cstddef>

namespace incubator::storage
{
    class NvsStorage
    {
    public:
        static constexpr const char* kNamespace = "incubator";
        static constexpr const char* kKeySettings   = "settings";
        static constexpr const char* kKeyBatch      = "batch";
        static constexpr const char* kKeyBootCount  = "boot_cnt";
        static constexpr const char* kKeyRtState    = "rt_state";

        bool init();

        bool saveBlob(const char* key, const void* data, size_t size);
        bool loadBlob(const char* key, void* data, size_t expectedSize);

        bool saveU32(const char* key, uint32_t value);
        bool loadU32(const char* key, uint32_t& outValue);

        bool eraseKey(const char* key);
        bool eraseAll();

        bool isInitialized() const { return m_open; }

    private:
        nvs_handle_t m_handle = 0;
        bool         m_open   = false;

        bool openRW();
        void close();
    };
}
