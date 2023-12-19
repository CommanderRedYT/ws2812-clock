#include "ota.h"

constexpr const char * const TAG = "OTA";

// esp-idf includes
#include <esp_image_format.h>
#include <esp_log.h>
#include <esp_ota_ops.h>

// 3rdparty lib includes
#include <delayedconstruction.h>
#include <espasyncota.h>
#include <recursivelockhelper.h>
#include <cpputils.h>

// local includes
#include "utils/global_lock.h"

namespace ota {

namespace {

cpputils::DelayedConstruction<EspAsyncOta> asyncOta;

} // namespace

std::optional<esp_app_desc_t> otherAppDesc;

void begin()
{
    asyncOta.construct("asyncOtaTask", 8192u, espcpputils::CoreAffinity::Both);

    if (const auto res = readAppInfo(); !res)
        ESP_LOGE(TAG, "Failed to read app info: %s", res.error().c_str());
}

void update()
{
    if (!asyncOta)
        return;

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    asyncOta->update();
}

std::expected<void, std::string> trigger(std::string_view url)
{
    if (!asyncOta)
        return std::unexpected("OTA not initialized");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    return asyncOta->trigger(url, {}, true, {}, {});
}

std::expected<void, std::string> switchAppPartition()
{
    auto partition = esp_ota_get_boot_partition();

    if (!partition)
    {
        ESP_LOGE(TAG, "Failed to get boot partition");
        return std::unexpected("Failed to get boot partition");
    }

    partition = esp_ota_get_next_update_partition(partition);

    if (!partition)
    {
        ESP_LOGE(TAG, "Failed to get next update partition");
        return std::unexpected("Failed to get next update partition");
    }

    const esp_partition_pos_t partitionPos = {
            .offset = partition->address,
            .size = partition->size,
    };

    esp_image_metadata_t metadata;
    if (const auto res = esp_image_verify(ESP_IMAGE_VERIFY, &partitionPos, &metadata); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to verify image: %s", esp_err_to_name(res));
        return std::unexpected("Failed to verify image");
    }

    if (const auto res = esp_ota_set_boot_partition(partition); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(res));
        return std::unexpected("Failed to set boot partition");
    }

    ESP_LOGI(TAG, "Switched to partition %s", partition->label);

    return {};
}

std::expected<void, std::string> readAppInfo()
{
    otherAppDesc = std::nullopt;

    auto partition = esp_ota_get_boot_partition();

    if (!partition)
    {
        ESP_LOGE(TAG, "Failed to get boot partition");
        return std::unexpected("Failed to get boot partition");
    }

    partition = esp_ota_get_next_update_partition(partition);

    if (!partition)
    {
        ESP_LOGE(TAG, "Failed to get next update partition");
        return std::unexpected("Failed to get next update partition");
    }

    const esp_partition_pos_t partitionPos = {
            .offset = partition->address,
            .size = partition->size,
    };

    esp_image_metadata_t metadata;
    if (const auto res = esp_image_verify(ESP_IMAGE_VERIFY, &partitionPos, &metadata); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to verify image: %s", esp_err_to_name(res));
        return std::unexpected("Failed to verify image");
    }

    esp_app_desc_t appDesc;
    if (const auto res = esp_ota_get_partition_description(partition, &appDesc); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get partition description: %s", esp_err_to_name(res));
        return std::unexpected("Failed to get partition description");
    }

    otherAppDesc = appDesc;

    return {};
}

bool isInProgress()
{
    return !cpputils::is_in(asyncOta->status(), OtaCloudUpdateStatus::Idle, OtaCloudUpdateStatus::NotReady);
}

int progress()
{
    if (!asyncOta)
        return 0;

    return asyncOta->progress();
}

std::optional<uint32_t> totalSize()
{
    if (!asyncOta)
        return std::nullopt;

    return asyncOta->totalSize();
}

float percent()
{
    if (!asyncOta)
        return 0.0f;

    // calculate based on progress and total size
    if (const auto totalSize = asyncOta->totalSize(); totalSize)
        return static_cast<float>(asyncOta->progress()) / static_cast<float>(*totalSize) * 100.0f;

    return 0.0f;
}

const std::string& otaMessage()
{
    static std::string emptyString;
    if (!asyncOta)
        return emptyString;

    return asyncOta->message();
}

const std::optional<esp_app_desc_t>& otaAppDesc()
{
    static const std::optional<esp_app_desc_t> emptyAppDesc;
    if (!asyncOta)
        return emptyAppDesc;

    return asyncOta->appDesc();
}

bool isConstructed()
{
    return asyncOta;
}

} // namespace ota
