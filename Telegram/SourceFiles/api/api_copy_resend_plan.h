/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#pragma once

#include "base/basic_types.h"

#include <span>
#include <vector>

namespace Api::CopyResend {

enum class ContentKind : uchar {
	Text,
	Photo,
	Document,
	Video,
	Voice,
	RoundVideo,
	Sticker,
	RichMessage,
	Unsupported,
};

enum class Error : uchar {
	None,
	Disabled,
	EmptySelection,
	NoProtectedContent,
	ServiceMessage,
	ExpiringMedia,
	UnsupportedMedia,
	TooManyItems,
	TemporaryStorageLimit,
};

struct SourceDescriptor {
	ContentKind kind = ContentKind::Unsupported;
	uint64 groupId = 0;
	int64 bytes = 0;
	bool protectedContent = false;
	bool service = false;
	bool expiring = false;
};

struct PlannedItem {
	ContentKind kind = ContentKind::Unsupported;
	uint64 groupId = 0;
	int64 bytes = 0;
};

struct Plan {
	std::vector<PlannedItem> items;
	Error error = Error::None;
	int64 temporaryBytes = 0;

	[[nodiscard]] explicit operator bool() const {
		return error == Error::None;
	}
};

inline constexpr auto kMaxItems = 100;
inline constexpr auto kMaxTemporaryBytes = int64(2) * 1024 * 1024 * 1024;

[[nodiscard]] Plan BuildPlan(
	std::span<const SourceDescriptor> sources,
	bool enabled,
	int maxItems = kMaxItems,
	int64 maxTemporaryBytes = kMaxTemporaryBytes);

} // namespace Api::CopyResend
