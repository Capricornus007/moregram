/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#include "core/appearance_policy.h"

#include "settings.h"
#include "ui/style/style_core_policy.h"

#include <QtCore/QByteArray>

#include <algorithm>
#include <charconv>

namespace Appearance {
namespace {

RenderPolicy RenderPolicyValue;
FontPolicy FontPolicyValue;

[[nodiscard]] std::optional<int> StoredWeight(int value) {
	switch (value) {
	case 0: return std::nullopt;
	case 1: return 300;
	case 2: return 400;
	case 3: return 500;
	case 4: return 600;
	}
	return std::nullopt;
}
[[nodiscard]] bool ReadBool(const char *key, bool fallback) {
	const auto name = QString::fromLatin1(key);
	if (!gEnhancedOptions.contains(name)) {
		SetEnhancedValue(name, fallback);
		return fallback;
	}
	return GetEnhancedBool(name);
}

[[nodiscard]] int ReadInt(const char *key, int fallback) {
	const auto name = QString::fromLatin1(key);
	if (!gEnhancedOptions.contains(name)) {
		SetEnhancedValue(name, fallback);
		return fallback;
	}
	return GetEnhancedInt(name);
}


[[nodiscard]] SettingsInput ReadSettings() {
	return {
		.materialMode = ReadBool("material_appearance", false),
		.corners = static_cast<CornerPolicy>(std::clamp(
			ReadInt("appearance_corner_policy", 0), 0, 2)),
		.density = static_cast<Density>(std::clamp(
			ReadInt("appearance_density", 0), 0, 2)),
		.capsuleButtons = ReadBool("appearance_capsule_buttons", false),
		.capsuleSidebar = ReadBool("appearance_capsule_sidebar", false),
		.bubbleTails = ReadBool("appearance_bubble_tails", true),
		.largeEmojiOutline = ReadBool("appearance_large_emoji_outline", true),
		.exactFontSize = ReadBool("font_exact_size", false),
		.avoidSyntheticBold = ReadBool("font_avoid_synthetic_bold", false),
		.normalFontWeight = ReadInt("font_normal_weight", 0),
	};
}

} // namespace

std::optional<int> ParseUiFontWeight(std::string_view value) {
	while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
		value.remove_prefix(1);
	}
	while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
		value.remove_suffix(1);
	}
	if (value.empty()) {
		return std::nullopt;
	}
	auto parsed = 0;
	const auto result = std::from_chars(
		value.data(),
		value.data() + value.size(),
		parsed);
	if (result.ec != std::errc()
		|| result.ptr != value.data() + value.size()
		|| parsed < 100
		|| parsed > 900) {
		return std::nullopt;
	}
	return parsed;
}

RenderPolicy ResolveRenderPolicy(const SettingsInput &input) {
	const auto density = (input.density == Density::Compact)
		? 90
		: (input.density == Density::Comfortable)
		? 110
		: 100;
	const auto radius = (input.corners == CornerPolicy::Square)
		? 1
		: (input.corners == CornerPolicy::Material || input.materialMode)
		? 16
		: 0;
	return {
		.roundButtonRadius = radius,
		.densityPercent = density,
		.largeEmojiOutline = input.largeEmojiOutline ? 1 : 0,
		.capsuleButtons = input.capsuleButtons,
		.capsuleSidebar = input.capsuleSidebar,
		.bubbleTails = input.bubbleTails,
	};
}

FontPolicy ResolveFontPolicy(
		const SettingsInput &input,
		std::string_view environmentWeight) {
	const auto environment = ParseUiFontWeight(environmentWeight);
	return {
		.normalWeight = environment
			? environment
			: StoredWeight(input.normalFontWeight),
		.weightFromEnvironment = environment.has_value(),
		.exactSize = input.exactFontSize,
		.avoidSyntheticBold = input.avoidSyntheticBold,
	};
}

void ApplyStartupPolicy() {
	const auto input = ReadSettings();
	const auto environment = qgetenv("TELEGRAM_DESKTOP_UI_FONT_WEIGHT");
	RenderPolicyValue = ResolveRenderPolicy(input);
	FontPolicyValue = ResolveFontPolicy(input, environment.isEmpty()
		? std::string_view()
		: std::string_view(environment.constData(), environment.size()));
	style::SetUiPolicy({
		.roundButtonRadius = RenderPolicyValue.roundButtonRadius,
		.densityPercent = RenderPolicyValue.densityPercent,
		.roundButtonCapsule = RenderPolicyValue.capsuleButtons,
		.sideBarCapsule = RenderPolicyValue.capsuleSidebar,
	});
	style::SetFontPolicy({
		.normalWeight = FontPolicyValue.normalWeight,
		.exactSize = FontPolicyValue.exactSize,
		.avoidSyntheticBold = FontPolicyValue.avoidSyntheticBold,
	});
}

const RenderPolicy &Current() {
	return RenderPolicyValue;
}

const FontPolicy &CurrentFont() {
	return FontPolicyValue;
}

int LargeEmojiOutline() {
	return RenderPolicyValue.largeEmojiOutline;
}

} // namespace Appearance
