#pragma once

// system includes
#include <optional>
#include <string>
#include <expected>

// esp-idf includes
#include <esp_app_desc.h>

namespace ota {

extern std::optional<esp_app_desc_t> otherAppDesc;

void begin();

void update();

std::expected<void, std::string> trigger(std::string_view url);

std::expected<void, std::string> switchAppPartition();

std::expected<void, std::string> readAppInfo();

bool isInProgress();

int progress();

std::optional<uint32_t> totalSize();

float percent();

const std::string& otaMessage();

const std::optional<esp_app_desc_t>& otaAppDesc();

bool isConstructed();

} // namespace ota
