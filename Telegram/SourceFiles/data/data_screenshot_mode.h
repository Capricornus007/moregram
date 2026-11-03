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

[[nodiscard]] QString PseudonymToken(uint64 sessionSalt, uint64 identity);
[[nodiscard]] QString PeerName(
	PeerKind kind,
	const void *sessionIdentity,
	uint64 identity);
[[nodiscard]] QString HiddenPeerName(PeerKind kind, QStringView identity);

} // namespace Data::Screenshot
