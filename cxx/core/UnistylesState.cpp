#include "UnistylesState.h"
#include "UnistylesRegistry.h"

using namespace margelo::nitro::unistyles;

bool core::UnistylesState::hasAdaptiveThemes() {
    if (!this->_prefersAdaptiveThemes.has_value() || !this->_prefersAdaptiveThemes.value()) {
        return false;
    }

    return helpers::vecContainsKeys(this->_registeredThemeNames, {"light", "dark"});
}

void core::UnistylesState::setTheme(std::string themeName) {
    helpers::assertThat(*_rt, helpers::vecContainsKeys(this->_registeredThemeNames, {themeName}), "Unistyles: You're trying to set theme to: '" + std::string(themeName) + "', but it wasn't registered.");

    if (themeName != this->_currentThemeName) {
        this->_currentThemeName = themeName;
    }
}

std::optional<std::string>& core::UnistylesState::getCurrentThemeName() {
    return this->_currentThemeName;
}

jsi::Object core::UnistylesState::getCurrentJSTheme() {
    auto hasSomeThemes = _registeredThemeNames.size() > 0;

    if (!hasSomeThemes && !this->hasUserConfig) {
        helpers::assertThat(*_rt, false, "Unistyles: One of your stylesheets is trying to get the theme, but no theme has been selected yet. Did you forget to call StyleSheet.configure? If you called it, make sure you did so before any StyleSheet.create.");
    }

    // return empty object, if user didn't register any themes
    if (!hasSomeThemes) {
        return jsi::Object(*_rt);
    }

    helpers::assertThat(*_rt, _currentThemeName.has_value(), "Unistyles: One of your stylesheets is trying to get the theme, but no theme has been selected yet. Did you forget to select an initial theme?");

    auto it = this->_jsThemes.find(_currentThemeName.value());

    helpers::assertThat(*_rt, it != this->_jsThemes.end(), "Unistyles: You're trying to get theme '" + _currentThemeName.value() + "', but it was not registered. Did you forget to register it with StyleSheet.configure?");

    return it->second.asObject(*_rt);
}

jsi::Object core::UnistylesState::getJSThemeByName(std::string& themeName) {
    auto it = this->_jsThemes.find(themeName);

    helpers::assertThat(*_rt, it != this->_jsThemes.end(), "Unistyles: You're trying to get theme '" + themeName + "', but it was not registered. Did you forget to register it with StyleSheet.configure?");

    return it->second.asObject(*_rt);
}

void core::UnistylesState::computeCurrentBreakpoint(int screenWidth) {
    if (this->_sortedBreakpointPairs.size() == 0) {
        return;
    }

    this->_currentBreakpointName = helpers::getBreakpointFromScreenWidth(
        screenWidth,
        this->_sortedBreakpointPairs
    );
}

bool core::UnistylesState::hasTheme(std::string themeName) {
    return helpers::vecContainsKeys(this->_registeredThemeNames, {themeName});
}

bool core::UnistylesState::hasInitialTheme() {
    return this->_initialThemeName.has_value();
}

std::vector<std::string> core::UnistylesState::getRegisteredThemeNames() {
    return std::vector<std::string>(this->_registeredThemeNames);
}

std::vector<std::pair<std::string, double>> core::UnistylesState::getSortedBreakpointPairs() {
    return std::vector<std::pair<std::string, double>>(this->_sortedBreakpointPairs);
}

std::optional<std::string> core::UnistylesState::getInitialTheme() {
    return this->_initialThemeName;
}

std::optional<std::string> core::UnistylesState::getCurrentBreakpointName() {
    return this->_currentBreakpointName;
}

bool core::UnistylesState::getPrefersAdaptiveThemes() {
    return this->_prefersAdaptiveThemes.has_value() && this->_prefersAdaptiveThemes.value();
}

void core::UnistylesState::registerProcessColorFunction(jsi::Function&& fn) {
    this->_processColorFn = std::make_shared<jsi::Function>(std::move(fn));
}

void core::UnistylesState::registerParseBoxShadowString(jsi::Function&& fn) {
    this->_parseBoxShadowStringFn = std::make_shared<jsi::Function>(std::move(fn));
}

int core::UnistylesState::parseColor(jsi::Value& maybeColor) {
    if (!maybeColor.isString()) {
        return 0;
    }

    auto colorString = maybeColor.asString(*_rt);

    if (!this->_colorCache.contains(colorString.utf8(*_rt).c_str())) {
        #ifdef ANDROID
            int color = this->_processColorFn.get()->call(*_rt, colorString).asNumber();
        #else
            uint32_t color = this->_processColorFn.get()->call(*_rt, colorString).asNumber();
        #endif

        this->_colorCache[colorString.utf8(*_rt).c_str()] = color ? color : 0;
    }

    return this->_colorCache[colorString.utf8(*_rt).c_str()];
}

jsi::Array core::UnistylesState::parseBoxShadowString(std::string&& boxShadowString) {
    jsi::Value result = this->_parseBoxShadowStringFn.get()->call(*_rt, boxShadowString);

    return result.asObject(*_rt).asArray(*_rt);
}
