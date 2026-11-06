#include "../test.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "jeson/jeson.hpp"

namespace {

class CompactJesonParseResult final : public ParseResultBase {
public:
    jeson::CompactDocument document;
};

class CompactJesonStringResult final : public StringResultBase {
public:
    const char* c_str() const override { return value.c_str(); }

    std::string value;
};

class CompactJesonTest final : public TestBase {
public:
#if TEST_INFO
    const char* GetName() const override { return "JESON CompactDocument (C++20)"; }
    const char* GetFilename() const override { return __FILE__; }
#endif

#if TEST_PARSE
    ParseResultBase* Parse(const char* json, size_t length) const override {
        auto* result = new CompactJesonParseResult;
        if (!jeson::parse(std::string_view(json, length), result->document)) {
            delete result;
            return nullptr;
        }
        return result;
    }
#endif

#if TEST_STRINGIFY
    StringResultBase* Stringify(const ParseResultBase* parseResult) const override {
        const auto* parsed = static_cast<const CompactJesonParseResult*>(parseResult);
        auto* result = new CompactJesonStringResult;
        result->value = jeson::toJson(parsed->document);
        return result;
    }
#endif

#if TEST_PRETTIFY
    StringResultBase* Prettify(const ParseResultBase* parseResult) const override {
        const auto* parsed = static_cast<const CompactJesonParseResult*>(parseResult);
        auto* result = new CompactJesonStringResult;
        result->value = jeson::toJson(parsed->document, jeson::pretty);
        return result;
    }
#endif

#if TEST_STATISTICS
    bool Statistics(const ParseResultBase* parseResult, Stat* stat) const override {
        const auto* parsed = static_cast<const CompactJesonParseResult*>(parseResult);
        const auto source = parsed->document.statistics();
        std::memset(stat, 0, sizeof(*stat));
        stat->objectCount = source.objectCount;
        stat->arrayCount = source.arrayCount;
        stat->numberCount = source.numberCount;
        stat->stringCount = source.stringCount;
        stat->trueCount = source.trueCount;
        stat->falseCount = source.falseCount;
        stat->nullCount = source.nullCount;
        stat->memberCount = source.memberCount;
        stat->elementCount = source.elementCount;
        stat->stringLength = source.stringLength;
        return true;
    }
#endif

#if TEST_CONFORMANCE
    bool ParseDouble(const char* json, double* value) const override {
        jeson::CompactDocument document;
        if (!jeson::parse(json, document)) return false;
        const auto item = document.root()[0];
        if (!item.valid() || document.root().size() != 1 || !item.isNumber()) return false;
        *value = item.doubleValue();
        return true;
    }

    bool ParseString(const char* json, std::string& value) const override {
        jeson::CompactDocument document;
        if (!jeson::parse(json, document)) return false;
        const auto item = document.root()[0];
        if (!item.valid() || document.root().size() != 1 || !item.isString()) return false;
        value.assign(item.stringValue());
        return true;
    }
#endif
};

} // namespace

REGISTER_TEST(CompactJesonTest);
