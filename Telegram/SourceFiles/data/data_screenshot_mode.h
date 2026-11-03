/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#pragma once

#include "base/basic_types.h"

#include <QtCore/QString>
#include <QtCore/QStringView>

namespace Data::Screenshot {

enum class PeerKind : uchar {
	User,
	Bot,
	Channel,
	Forum,
	Group,
	Chat,
};

[[nodiscard]] inline QString PseudonymToken(uint64 sessionSalt, uint64 identity) {
	const auto mix = [](uint64 value) {
		value ^= value >> 30;
		value *= 0xbf58476d1ce4e5b9ULL;
		value ^= value >> 27;
		value *= 0x94d049bb133111ebULL;
		return value ^ (value >> 31);
	};
	const auto mixed = mix(identity ^ mix(sessionSalt));
	return QString::number(mixed, 36).toUpper().rightJustified(13, QChar('0'));
}
[[nodiscard]] QString PeerName(
	PeerKind kind,
	const void *sessionIdentity,
	uint64 identity);
[[nodiscard]] QString HiddenPeerName(PeerKind kind, QStringView identity);

} // namespace Data::Screenshot
