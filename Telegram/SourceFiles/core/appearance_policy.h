/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#pragma once

#include <optional>
#include <string_view>

namespace Appearance {

enum class Density {
	Upstream,
	Compact,
	Comfortable,
};

enum class CornerPolicy {
	Upstream,
	Material,
	Square,
};

struct SettingsInput final {
	bool materialMode = false;
	CornerPolicy corners = CornerPolicy::Upstream;
	Density density = Density::Upstream;
	bool capsuleButtons = false;
	bool capsuleSidebar = false;
	bool bubbleTails = true;
	bool largeEmojiOutline = true;
	bool exactFontSize = false;
	bool avoidSyntheticBold = false;
	int normalFontWeight = 0;
};

struct RenderPolicy final {
	int roundButtonRadius = 0;
	int densityPercent = 100;
	int largeEmojiOutline = 1;
	bool capsuleButtons = false;
	bool capsuleSidebar = false;
	bool bubbleTails = true;
};

struct FontPolicy final {
	std::optional<int> normalWeight;
	bool weightFromEnvironment = false;
	bool exactSize = false;
	bool avoidSyntheticBold = false;
};

[[nodiscard]] std::optional<int> ParseUiFontWeight(std::string_view value);
[[nodiscard]] RenderPolicy ResolveRenderPolicy(const SettingsInput &input);
[[nodiscard]] FontPolicy ResolveFontPolicy(
	const SettingsInput &input,
	std::string_view environmentWeight);

void ApplyStartupPolicy();
[[nodiscard]] const RenderPolicy &Current();
[[nodiscard]] const FontPolicy &CurrentFont();
[[nodiscard]] int LargeEmojiOutline();

} // namespace Appearance
