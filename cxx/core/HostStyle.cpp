#include "HostStyle.h"

using namespace margelo::nitro::unistyles::core;
using namespace margelo::nitro::unistyles::parser;
using namespace facebook;

std::vector<jsi::PropNameID> HostStyle::getPropertyNames(jsi::Runtime& rt) {
    auto propertyNames = std::vector<jsi::PropNameID> {};

    propertyNames.reserve(8);

    for (const auto& pair : this->_styleSheet->unistyles) {
        propertyNames.emplace_back(jsi::PropNameID::forUtf8(rt, pair.first));
    }

    return propertyNames;
}

jsi::Value HostStyle::get(jsi::Runtime& rt, const jsi::PropNameID& propNameId) {
    auto propertyName = propNameId.utf8(rt);

    if (propertyName == helpers::UNISTYLES_ID) {
        return jsi::Value(this->_styleSheet->tag);
    }

    if (propertyName == helpers::ADD_VARIANTS_FN) {
        return this->createAddVariantsProxyFunction(rt);
    }

    if (this->_styleSheet->unistyles.contains(propertyName)) {
        return valueFromUnistyle(rt, this->_unistylesRuntime, this->_styleSheet->unistyles[propertyName], this->_styleSheet->tag);
    }

    if (propertyName == helpers::STYLE_VARIANTS) {
        return helpers::variantsToValue(rt, this->_variants);
    }

    return jsi::Value::undefined();
}

jsi::Function HostStyle::createAddVariantsProxyFunction(jsi::Runtime& rt) {
    auto useVariantsFnName = jsi::PropNameID::forUtf8(rt, helpers::ADD_VARIANTS_FN);

    return jsi::Function::createFromHostFunction(rt, useVariantsFnName, 1, [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *arguments, size_t count){
        helpers::assertThat(rt, count == 1, "Unistyles: useVariants expected to be called with one argument.");
        helpers::assertThat(rt, arguments[0].isObject(), "Unistyles: useVariants expected to be called with object.");

        this->_variants = helpers::variantsToPairs(rt, arguments[0].asObject(rt));

        return jsi::Value::undefined();
    });
}

void HostStyle::set(jsi::Runtime& rt, const jsi::PropNameID& propNameId, const jsi::Value& value) {}
