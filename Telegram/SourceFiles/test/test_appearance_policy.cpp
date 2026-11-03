/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#ifdef _DEBUG

#include "test/test_appearance_policy.h"

#include "core/appearance_policy.h"
#include "test/test_log.h"
#include "test/test_runner.h"
#include "ui/style/style_core_policy.h"

namespace Test {
namespace {

void CheckDefaults() {
	const auto policy = Appearance::ResolveRenderPolicy({
		.bubbleTails = true,
		.largeEmojiOutline = true,
	});
	Check(policy.roundButtonRadius == 0
		&& policy.densityPercent == 100
		&& policy.largeEmojiOutline == 1
		&& !policy.capsuleButtons
		&& !policy.capsuleSidebar
		&& policy.bubbleTails,
		u"appearance policy defaults preserve upstream"_q);
}

void CheckMaterialAndOverrides() {
	const auto material = Appearance::ResolveRenderPolicy({
		.materialMode = true,
		.bubbleTails = true,
		.largeEmojiOutline = true,
	});
	Check(material.roundButtonRadius == 16
		&& !material.capsuleButtons
		&& !material.capsuleSidebar
		&& material.bubbleTails
		&& material.largeEmojiOutline == 1,
		u"material mode is opt-in and keeps other controls independent"_q);
	const auto personalized = Appearance::ResolveRenderPolicy({
		.corners = Appearance::CornerPolicy::Square,
		.density = Appearance::Density::Compact,
		.capsuleSidebar = true,
		.bubbleTails = false,
		.largeEmojiOutline = false,
	});
	Check(personalized.roundButtonRadius == 1
		&& personalized.densityPercent == 90
		&& !personalized.capsuleButtons
		&& personalized.capsuleSidebar
		&& !personalized.bubbleTails
		&& personalized.largeEmojiOutline == 0,
		u"appearance controls resolve independently"_q);
}

void CheckFontPolicy() {
	Check(Appearance::ParseUiFontWeight(" 500 ") == 500
		&& !Appearance::ParseUiFontWeight("")
		&& !Appearance::ParseUiFontWeight("99")
		&& !Appearance::ParseUiFontWeight("901")
		&& !Appearance::ParseUiFontWeight("500x"),
		u"font environment weight parser is bounded and complete"_q);
	const auto environment = Appearance::ResolveFontPolicy({
		.exactFontSize = true,
		.avoidSyntheticBold = true,
		.normalFontWeight = 1,
	}, "600");
	Check(environment.normalWeight == 600
		&& environment.weightFromEnvironment
		&& environment.exactSize
		&& environment.avoidSyntheticBold,
		u"font environment weight has one-startup precedence"_q);
	const auto stored = Appearance::ResolveFontPolicy({
		.normalFontWeight = 3,
	}, "invalid");
	Check(stored.normalWeight == 500 && !stored.weightFromEnvironment,
		u"stored normal font weight is the validated fallback"_q);
}

void CheckLibraryRenderPolicy() {
	const auto previous = style::CurrentUiPolicy();
	style::SetUiPolicy({
		.roundButtonRadius = 16,
		.densityPercent = 90,
		.roundButtonCapsule = false,
		.sideBarCapsule = true,
	});
	Check(style::RoundButtonRadius(6, 20) == 10
		&& style::RoundButtonRadius(6, 40) == 16
		&& style::AdjustUiDensity(40) == 36
		&& style::CurrentUiPolicy().sideBarCapsule,
		u"render policy clamps radii and applies density without state assets"_q);
	style::SetUiPolicy(previous);
}

} // namespace

void AppendAppearancePolicySelfTest(not_null<Runner*> runner) {
	for (const auto &[name, test] : std::vector<std::pair<QString, Fn<void()>>>{
		{ u"appearance_policy_defaults"_q, CheckDefaults },
		{ u"appearance_policy_material"_q, CheckMaterialAndOverrides },
		{ u"appearance_policy_font"_q, CheckFontPolicy },
		{ u"appearance_policy_render"_q, CheckLibraryRenderPolicy },
	}) {
		runner->add({
			.name = name,
			.then = test,
			.timeout = kDefaultStageTimeout,
		});
	}
}

} // namespace Test

#endif // _DEBUG
