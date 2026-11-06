#include "memorystat.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(TYPED_JESON)
#include "jeson/jeson.hpp"
#elif defined(TYPED_JESON_VALUE)
#include "jeson/jeson.hpp"
#elif defined(TYPED_JESON_COMPACT)
#include "jeson/jeson.hpp"
#elif defined(TYPED_REFLECTCPP)
#include <rfl/json/read.hpp>
#elif defined(TYPED_NLOHMANN)
#include "nlohmann/src/json.hpp"
#elif defined(TYPED_RAPIDJSON)
#include "rapidjson/document.h"
#elif defined(TYPED_ARDUINOJSON)
#include "ArduinoJson/include/ArduinoJson.hpp"
#elif defined(TYPED_CJSON)
#include "cJSON/cJSON.h"
#else
#error Select one typed benchmark adapter
#endif

namespace {

constexpr std::string_view INPUT =
    R"({"device_id":"sensor-01","sample_interval_ms":1000,"enabled":true,"threshold":21.5,"radio":{"channel":15,"tx_power":4},"labels":["lab","north","critical"],"retry_delays":[100,250,500,1000]})";

struct RadioModel {
    unsigned channel = 0;
    int tx_power = 0;
};

struct PlainModel {
    std::string device_id;
    unsigned sample_interval_ms = 0;
    bool enabled = false;
    double threshold = 0.0;
    RadioModel radio;
    std::vector<std::string> labels;
    std::vector<unsigned> retry_delays;
};

[[maybe_unused]] std::uint64_t checksum(const PlainModel& model) {
    std::uint64_t result = model.device_id.size() + model.sample_interval_ms + (model.enabled ? 17U : 0U);
    result += static_cast<std::uint64_t>(model.threshold * 10.0);
    result += model.radio.channel + static_cast<std::uint64_t>(model.radio.tx_power + 32);
    for (const auto& label : model.labels) result += label.size();
    for (const auto delay : model.retry_delays) result += delay;
    return result;
}

#if defined(TYPED_JESON)

struct JesonRadio {
    jeson::Field<"channel", unsigned> channel;
    jeson::Field<"tx_power", int> txPower;
};

struct ResultModel {
    jeson::Field<"device_id", std::string> deviceId;
    jeson::Field<"sample_interval_ms", unsigned> sampleIntervalMs;
    jeson::Field<"enabled", bool> enabled;
    jeson::Field<"threshold", double> threshold;
    jeson::Field<"radio", JesonRadio> radio;
    jeson::Field<"labels", std::vector<std::string>> labels;
    jeson::Field<"retry_delays", std::vector<unsigned>> retryDelays;
};

constexpr const char* LIBRARY = "JESON typed";
constexpr const char* MAPPING = "native direct-to-model";

bool parseModel(std::string_view json, ResultModel& model) {
    return jeson::parse(json, model).ok();
}

std::uint64_t checksum(const ResultModel& model) {
    std::uint64_t result = model.deviceId.value.size() + model.sampleIntervalMs.value + (model.enabled.value ? 17U : 0U);
    result += static_cast<std::uint64_t>(model.threshold.value * 10.0);
    result += model.radio.channel.value + static_cast<std::uint64_t>(model.radio.txPower.value + 32);
    for (const auto& label : model.labels.value) result += label.size();
    for (const auto delay : model.retryDelays.value) result += delay;
    return result;
}

#elif defined(TYPED_REFLECTCPP)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "reflect-cpp typed";
constexpr const char* MAPPING = "native direct-to-model";

bool parseModel(std::string_view json, ResultModel& model) {
    auto result = rfl::json::read<ResultModel>(json);
    if (!result) return false;
    model = std::move(result).value();
    return true;
}

#elif defined(TYPED_JESON_VALUE)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "JESON Mutable DOM";
constexpr const char* MAPPING = "Value + manual mapping";

bool parseModel(std::string_view json, ResultModel& model) {
    jeson::Value document;
    if (!jeson::parse(json, document) || !document.is<jeson::Value::Object>()) return false;
    const auto& object = document.as<jeson::Value::Object>();
    const auto find = [&](std::string_view key) -> const jeson::Value* {
        for (const auto& [name, value] : object) {
            if (name == key) return &value;
        }
        return nullptr;
    };
    const auto* deviceId = find("device_id");
    const auto* interval = find("sample_interval_ms");
    const auto* enabled = find("enabled");
    const auto* threshold = find("threshold");
    const auto* radio = find("radio");
    const auto* labels = find("labels");
    const auto* delays = find("retry_delays");
    if (!deviceId || !interval || !enabled || !threshold || !radio || !labels || !delays) return false;
    model.device_id = deviceId->as<std::string>();
    model.sample_interval_ms = static_cast<unsigned>(interval->as<std::uint64_t>());
    model.enabled = enabled->as<bool>();
    model.threshold = threshold->as<double>();
    const auto& radioObject = radio->as<jeson::Value::Object>();
    const auto findRadio = [&](std::string_view key) -> const jeson::Value* {
        for (const auto& [name, value] : radioObject) {
            if (name == key) return &value;
        }
        return nullptr;
    };
    const auto* channel = findRadio("channel");
    const auto* txPower = findRadio("tx_power");
    if (!channel || !txPower) return false;
    model.radio.channel = static_cast<unsigned>(channel->as<std::uint64_t>());
    model.radio.tx_power = static_cast<int>(txPower->as<std::uint64_t>());
    for (const auto& label : labels->as<jeson::Value::Array>()) model.labels.push_back(label.as<std::string>());
    for (const auto& delay : delays->as<jeson::Value::Array>()) {
        model.retry_delays.push_back(static_cast<unsigned>(delay.as<std::uint64_t>()));
    }
    return true;
}

#elif defined(TYPED_JESON_COMPACT)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "JESON Compact DOM";
constexpr const char* MAPPING = "CompactDocument + manual mapping";

bool parseModel(std::string_view json, ResultModel& model) {
    jeson::CompactDocument document;
    if (!jeson::parse(json, document)) return false;
    const auto root = document.root();
    model.device_id.assign(root["device_id"].stringValue());
    model.sample_interval_ms = static_cast<unsigned>(root["sample_interval_ms"].uint64Value());
    model.enabled = root["enabled"].boolValue();
    model.threshold = root["threshold"].doubleValue();
    model.radio.channel = static_cast<unsigned>(root["radio"]["channel"].uint64Value());
    model.radio.tx_power = static_cast<int>(root["radio"]["tx_power"].uint64Value());
    for (const auto label : root["labels"].children()) model.labels.emplace_back(label.stringValue());
    for (const auto delay : root["retry_delays"].children()) {
        model.retry_delays.push_back(static_cast<unsigned>(delay.uint64Value()));
    }
    return true;
}

#elif defined(TYPED_NLOHMANN)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "nlohmann/json";
constexpr const char* MAPPING = "DOM + manual mapping";

bool parseModel(std::string_view json, ResultModel& model) {
    try {
        const auto document = nlohmann::json::parse(json.data(), json.data() + json.size());
        model.device_id = document["device_id"].get<std::string>();
        model.sample_interval_ms = document["sample_interval_ms"].get<unsigned>();
        model.enabled = document["enabled"].get<bool>();
        model.threshold = document["threshold"].get<double>();
        model.radio.channel = document["radio"]["channel"].get<unsigned>();
        model.radio.tx_power = document["radio"]["tx_power"].get<int>();
        model.labels = document["labels"].get<std::vector<std::string>>();
        model.retry_delays = document["retry_delays"].get<std::vector<unsigned>>();
        return true;
    } catch (...) {
        return false;
    }
}

#elif defined(TYPED_RAPIDJSON)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "RapidJSON";
constexpr const char* MAPPING = "DOM + manual mapping";

bool parseModel(std::string_view json, ResultModel& model) {
    rapidjson::Document document;
    document.Parse(json.data(), json.size());
    if (document.HasParseError() || !document.IsObject()) return false;
    model.device_id.assign(document["device_id"].GetString(), document["device_id"].GetStringLength());
    model.sample_interval_ms = document["sample_interval_ms"].GetUint();
    model.enabled = document["enabled"].GetBool();
    model.threshold = document["threshold"].GetDouble();
    model.radio.channel = document["radio"]["channel"].GetUint();
    model.radio.tx_power = document["radio"]["tx_power"].GetInt();
    for (const auto& label : document["labels"].GetArray()) model.labels.emplace_back(label.GetString(), label.GetStringLength());
    for (const auto& delay : document["retry_delays"].GetArray()) model.retry_delays.push_back(delay.GetUint());
    return true;
}

#elif defined(TYPED_ARDUINOJSON)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "ArduinoJson";
constexpr const char* MAPPING = "DOM + manual mapping";

bool parseModel(std::string_view json, ResultModel& model) {
    std::vector<char> input(json.begin(), json.end());
    input.push_back('\0');
    ArduinoJson::DynamicJsonBuffer buffer;
    auto& document = buffer.parseObject(input.data());
    if (!document.success()) return false;
    model.device_id = document["device_id"].as<const char*>();
    model.sample_interval_ms = document["sample_interval_ms"].as<unsigned>();
    model.enabled = document["enabled"].as<bool>();
    model.threshold = document["threshold"].as<double>();
    model.radio.channel = document["radio"]["channel"].as<unsigned>();
    model.radio.tx_power = document["radio"]["tx_power"].as<int>();
    for (const auto& label : document["labels"].as<ArduinoJson::JsonArray&>()) model.labels.emplace_back(label.as<const char*>());
    for (const auto& delay : document["retry_delays"].as<ArduinoJson::JsonArray&>()) model.retry_delays.push_back(delay.as<unsigned>());
    return true;
}

#elif defined(TYPED_CJSON)

using ResultModel = PlainModel;
constexpr const char* LIBRARY = "cJSON";
constexpr const char* MAPPING = "DOM + manual mapping";

bool parseModel(std::string_view json, ResultModel& model) {
    cJSON* document = cJSON_ParseWithOpts(json.data(), nullptr, static_cast<cJSON_bool>(true));
    if (document == nullptr) return false;
    const auto get = [&](const cJSON* object, const char* name) { return cJSON_GetObjectItemCaseSensitive(object, name); };
    const cJSON* radio = get(document, "radio");
    model.device_id = get(document, "device_id")->valuestring;
    model.sample_interval_ms = static_cast<unsigned>(get(document, "sample_interval_ms")->valuedouble);
    model.enabled = cJSON_IsTrue(get(document, "enabled"));
    model.threshold = get(document, "threshold")->valuedouble;
    model.radio.channel = static_cast<unsigned>(get(radio, "channel")->valuedouble);
    model.radio.tx_power = get(radio, "tx_power")->valueint;
    for (const cJSON* item = get(document, "labels")->child; item != nullptr; item = item->next) model.labels.emplace_back(item->valuestring);
    for (const cJSON* item = get(document, "retry_delays")->child; item != nullptr; item = item->next) {
        model.retry_delays.push_back(static_cast<unsigned>(item->valuedouble));
    }
    cJSON_Delete(document);
    return true;
}

#endif

constexpr std::uint64_t EXPECTED_CHECKSUM = 3158;
volatile std::uint64_t checksumSink = 0;

bool runOnce(std::uint64_t& value) {
    ResultModel model;
    if (!parseModel(INPUT, model)) return false;
    value = checksum(model);
    return value == EXPECTED_CHECKSUM;
}

} // namespace

int main(int argc, char** argv) {
    const unsigned iterations = argc > 1 ? static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10)) : 20000U;
    std::uint64_t value = 0;
    if (!runOnce(value)) {
        std::fprintf(stderr, "%s failed semantic verification (checksum %llu, expected %llu)\n", LIBRARY,
                     static_cast<unsigned long long>(value), static_cast<unsigned long long>(EXPECTED_CHECKSUM));
        return 1;
    }

    MemoryStat memory{};
    MemoryStat* previous = Memory::Instance().SetStat(&memory);
    if (!runOnce(value)) return 1;
    Memory::Instance().SetStat(previous);

    double bestMilliseconds = 1.0e30;
    for (unsigned trial = 0; trial < 5; ++trial) {
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t total = 0;
        for (unsigned iteration = 0; iteration < iterations; ++iteration) {
            if (!runOnce(value)) return 1;
            total += value;
        }
        const auto end = std::chrono::steady_clock::now();
        checksumSink = total;
        const double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        if (elapsed < bestMilliseconds) bestMilliseconds = elapsed;
    }

    const double microsecondsPerParse = bestMilliseconds * 1000.0 / iterations;
    std::printf("library,mapping,iterations,parse_us,peak_bytes,alloc_count,checksum\n");
    std::printf("%s,%s,%u,%.6f,%zu,%zu,%llu\n", LIBRARY, MAPPING, iterations, microsecondsPerParse,
                memory.peakSize, memory.mallocCount + memory.reallocCount,
                static_cast<unsigned long long>(value));
    return 0;
}
