#include "../test.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "jeson/jeson.hpp"

namespace {

void GenStat(Stat& stat, const jeson::Value& value) {
    std::visit(
        [&](const auto& current) {
            using T = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<T, jeson::Value::Null>) {
                ++stat.nullCount;
            } else if constexpr (std::is_same_v<T, bool>) {
                current ? ++stat.trueCount : ++stat.falseCount;
            } else if constexpr (std::is_arithmetic_v<T>) {
                ++stat.numberCount;
            } else if constexpr (std::is_same_v<T, std::string>) {
                ++stat.stringCount;
                stat.stringLength += current.size();
            } else if constexpr (std::is_same_v<T, jeson::Value::Array>) {
                for (const auto& element : current) {
                    GenStat(stat, element);
                }
                ++stat.arrayCount;
                stat.elementCount += current.size();
            } else if constexpr (std::is_same_v<T, jeson::Value::Object>) {
                for (const auto& member : current) {
                    ++stat.stringCount;
                    stat.stringLength += member.first.size();
                    GenStat(stat, member.second);
                }
                ++stat.objectCount;
                stat.memberCount += current.size();
            }
        },
        value.storage());
}

class JesonParseResult final : public ParseResultBase {
public:
    jeson::Value value;
};

class JesonStringResult final : public StringResultBase {
public:
    const char* c_str() const override { return value.c_str(); }

    std::string value;
};

class JesonTest final : public TestBase {
public:
#if TEST_INFO
    const char* GetName() const override { return "JESON Value (C++20)"; }
    const char* GetFilename() const override { return __FILE__; }
#endif

#if TEST_PARSE
    ParseResultBase* Parse(const char* json, size_t length) const override {
        auto* result = new JesonParseResult;
        if (!jeson::parse(std::string_view(json, length), result->value)) {
            delete result;
            return nullptr;
        }
        return result;
    }
#endif

#if TEST_STRINGIFY
    StringResultBase* Stringify(const ParseResultBase* parseResult) const override {
        const auto* parsed = static_cast<const JesonParseResult*>(parseResult);
        auto* result = new JesonStringResult;
        result->value = jeson::toJson(parsed->value);
        return result;
    }
#endif

#if TEST_PRETTIFY
    StringResultBase* Prettify(const ParseResultBase* parseResult) const override {
        const auto* parsed = static_cast<const JesonParseResult*>(parseResult);
        auto* result = new JesonStringResult;
        result->value = jeson::toJson(parsed->value, jeson::pretty);
        return result;
    }
#endif

#if TEST_STATISTICS
    bool Statistics(const ParseResultBase* parseResult, Stat* stat) const override {
        const auto* parsed = static_cast<const JesonParseResult*>(parseResult);
        memset(stat, 0, sizeof(*stat));
        GenStat(*stat, parsed->value);
        return true;
    }
#endif

#if TEST_CONFORMANCE
    bool ParseDouble(const char* json, double* value) const override {
        jeson::Value parsed;
        if (!jeson::parse(json, parsed) || !parsed.is<jeson::Value::Array>()) {
            return false;
        }
        const auto& values = parsed.as<jeson::Value::Array>();
        if (values.size() != 1) {
            return false;
        }
        const auto& item = values.front();
        if (item.is<double>()) {
            *value = item.as<double>();
        } else if (item.is<std::int64_t>()) {
            *value = static_cast<double>(item.as<std::int64_t>());
        } else if (item.is<std::uint64_t>()) {
            *value = static_cast<double>(item.as<std::uint64_t>());
        } else {
            return false;
        }
        return true;
    }

    bool ParseString(const char* json, std::string& value) const override {
        jeson::Value parsed;
        if (!jeson::parse(json, parsed) || !parsed.is<jeson::Value::Array>()) {
            return false;
        }
        const auto& values = parsed.as<jeson::Value::Array>();
        if (values.size() != 1 || !values.front().is<std::string>()) {
            return false;
        }
        value = values.front().as<std::string>();
        return true;
    }
#endif
};

} // namespace

REGISTER_TEST(JesonTest);
