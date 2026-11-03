/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#include "api/api_copy_resend_plan.h"
#include "data/data_screenshot_mode.h"

#include <array>

namespace {

[[nodiscard]] bool PseudonymsAreStableAndDistinct() {
	using Data::Screenshot::PseudonymToken;
	const auto first = PseudonymToken(0xA11CE, 1001);
	const auto again = PseudonymToken(0xA11CE, 1001);
	const auto second = PseudonymToken(0xA11CE, 1002);
	const auto otherSession = PseudonymToken(0xB0B, 1001);
	return first == again
		&& first != second
		&& first != otherSession
		&& !first.contains(u"1001"_q)
		&& first.size() == 13;
}

[[nodiscard]] bool ResendPlanningCoversSupportedContent() {
	using namespace Api::CopyResend;
	const auto sources = std::array{
		SourceDescriptor{ .kind = ContentKind::Text, .protectedContent = true },
		SourceDescriptor{ .kind = ContentKind::Photo, .groupId = 7, .bytes = 1024 },
		SourceDescriptor{ .kind = ContentKind::Video, .groupId = 7, .bytes = 2048 },
		SourceDescriptor{ .kind = ContentKind::Document, .bytes = 4096 },
		SourceDescriptor{ .kind = ContentKind::Voice, .bytes = 512 },
		SourceDescriptor{ .kind = ContentKind::RoundVideo, .bytes = 768 },
		SourceDescriptor{ .kind = ContentKind::Sticker },
		SourceDescriptor{ .kind = ContentKind::RichMessage },
	};
	const auto plan = BuildPlan(sources, true);
	return bool(plan)
		&& plan.items.size() == sources.size()
		&& plan.items[1].groupId == 7
		&& plan.items[2].groupId == 7
		&& plan.temporaryBytes == 8448;
}

[[nodiscard]] bool ResendPlanningRejectsUnsafeCases() {
	using namespace Api::CopyResend;
	const auto protectedText = SourceDescriptor{
		.kind = ContentKind::Text,
		.protectedContent = true,
	};
	const auto plainText = SourceDescriptor{ .kind = ContentKind::Text };
	const auto service = SourceDescriptor{
		.kind = ContentKind::Text,
		.protectedContent = true,
		.service = true,
	};
	const auto expiring = SourceDescriptor{
		.kind = ContentKind::Photo,
		.protectedContent = true,
		.expiring = true,
	};
	const auto unsupported = SourceDescriptor{
		.kind = ContentKind::Unsupported,
		.protectedContent = true,
	};
	const auto oversized = SourceDescriptor{
		.kind = ContentKind::Document,
		.bytes = kMaxTemporaryBytes + 1,
		.protectedContent = true,
	};
	return BuildPlan(std::span(&protectedText, 1), false).error == Error::Disabled
		&& BuildPlan(std::span(&plainText, 1), true).error == Error::NoProtectedContent
		&& BuildPlan(std::span(&service, 1), true).error == Error::ServiceMessage
		&& BuildPlan(std::span(&expiring, 1), true).error == Error::ExpiringMedia
		&& BuildPlan(std::span(&unsupported, 1), true).error == Error::UnsupportedMedia
		&& BuildPlan(std::span(&oversized, 1), true).error == Error::TemporaryStorageLimit;
}

} // namespace

int main() {
	return (PseudonymsAreStableAndDistinct()
		&& ResendPlanningCoversSupportedContent()
		&& ResendPlanningRejectsUnsafeCases()) ? 0 : 1;
}
