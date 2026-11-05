#include "../test.h"

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include <rfl/Generic.hpp>
#include <rfl/json/read.hpp>
#include <rfl/json/write.hpp>

namespace {

void GenStat(Stat& stat, const rfl::Generic& value) {
    std::visit(
        [&](const auto& current) {
            using T = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<T, std::nullopt_t>) {
                ++stat.nullCount;
            } else if constexpr (std::is_same_v<T, bool>) {
                current ? ++stat.trueCount : ++stat.falseCount;
            } else if constexpr (std::is_arithmetic_v<T>) {
                ++stat.numberCount;
            } else if constexpr (std::is_same_v<T, std::string>) {
                ++stat.stringCount;
                stat.stringLength += current.size();
            } else if constexpr (std::is_same_v<T, rfl::Generic::Array>) {
                for (const auto& element : current) {
                    GenStat(stat, element);
                }
                ++stat.arrayCount;
                stat.elementCount += current.size();
            } else if constexpr (std::is_same_v<T, rfl::Generic::Object>) {
                for (const auto& member : current) {
                    ++stat.stringCount;
                    stat.stringLength += member.first.size();
                    GenStat(stat, member.second);
                }
                ++stat.objectCount;
                stat.memberCount += current.size();
            }
        },
        value.get());
}

class ReflectCppParseResult final : public ParseResultBase {
public:
    rfl::Generic value;
};

class ReflectCppStringResult final : public StringResultBase {
public:
    const char* c_str() const override { return value.c_str(); }

    std::string value;
};

class ReflectCppTest final : public TestBase {
public:
#if TEST_INFO
    const char* GetName() const override { return "reflect-cpp Generic (JSON only)"; }
    const char* GetFilename() const override { return __FILE__; }
#endif

#if TEST_PARSE
    ParseResultBase* Parse(const char* json, size_t length) const override {
        auto parsed = rfl::json::read<rfl::Generic>(std::string_view(json, length));
        if (!parsed) {
            return nullptr;
        }
        auto* result = new ReflectCppParseResult;
        result->value = std::move(parsed).value();
        return result;
    }
#endif

#if TEST_STRINGIFY
    StringResultBase* Stringify(const ParseResultBase* parseResult) const override {
        const auto* parsed = static_cast<const ReflectCppParseResult*>(parseResult);
        auto* result = new ReflectCppStringResult;
        result->value = rfl::json::write(parsed->value);
        return result;
    }
#endif

#if TEST_PRETTIFY
    StringResultBase* Prettify(const ParseResultBase* parseResult) const override {
        const auto* parsed = static_cast<const ReflectCppParseResult*>(parseResult);
        auto* result = new ReflectCppStringResult;
        result->value = rfl::json::write(parsed->value, rfl::json::pretty);
        return result;
    }
#endif

#if TEST_STATISTICS
    bool Statistics(const ParseResultBase* parseResult, Stat* stat) const override {
        const auto* parsed = static_cast<const ReflectCppParseResult*>(parseResult);
        std::memset(stat, 0, sizeof(*stat));
        GenStat(*stat, parsed->value);
        return true;
    }
#endif

#if TEST_CONFORMANCE
    bool ParseDouble(const char* json, double* value) const override {
        auto parsed = rfl::json::read<rfl::Generic>(json);
        if (!parsed) {
            return false;
        }
        const auto* values = std::get_if<rfl::Generic::Array>(&parsed.value().get());
        if (values == nullptr || values->size() != 1) {
            return false;
        }
        const auto& item = values->front().get();
        if (const auto* number = std::get_if<double>(&item)) {
            *value = *number;
        } else if (const auto* number = std::get_if<std::int64_t>(&item)) {
            *value = static_cast<double>(*number);
        } else {
            return false;
        }
        return true;
    }

    bool ParseString(const char* json, std::string& value) const override {
        auto parsed = rfl::json::read<rfl::Generic>(json);
        if (!parsed) {
            return false;
        }
        const auto* values = std::get_if<rfl::Generic::Array>(&parsed.value().get());
        if (values == nullptr || values->size() != 1) {
            return false;
        }
        const auto* string = std::get_if<std::string>(&values->front().get());
        if (string == nullptr) {
            return false;
        }
        value = *string;
        return true;
    }
#endif
};

} // namespace

REGISTER_TEST(ReflectCppTest);
