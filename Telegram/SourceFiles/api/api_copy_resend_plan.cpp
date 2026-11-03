/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#include "api/api_copy_resend_plan.h"
#include "base/algorithm.h"


namespace Api::CopyResend {

Plan BuildPlan(
		std::span<const SourceDescriptor> sources,
		bool enabled,
		int maxItems,
		int64 maxTemporaryBytes) {
	auto result = Plan();
	if (!enabled) {
		result.error = Error::Disabled;
		return result;
	} else if (sources.empty()) {
		result.error = Error::EmptySelection;
		return result;
	} else if (sources.size() > size_t(maxItems)) {
		result.error = Error::TooManyItems;
		return result;
	}
	if (!ranges::any_of(sources, &SourceDescriptor::protectedContent)) {
		result.error = Error::NoProtectedContent;
		return result;
	}
	result.items.reserve(sources.size());
	for (const auto &source : sources) {
		if (source.service) {
			result.error = Error::ServiceMessage;
			return result;
		} else if (source.expiring) {
			result.error = Error::ExpiringMedia;
			return result;
		} else if (source.kind == ContentKind::Unsupported) {
			result.error = Error::UnsupportedMedia;
			return result;
		} else if (source.bytes < 0
			|| source.bytes > maxTemporaryBytes
			|| result.temporaryBytes > maxTemporaryBytes - source.bytes) {
			result.error = Error::TemporaryStorageLimit;
			return result;
		}
		result.temporaryBytes += source.bytes;
		result.items.push_back({
			.kind = source.kind,
			.groupId = source.groupId,
			.bytes = source.bytes,
		});
	}
	return result;
}

} // namespace Api::CopyResend
