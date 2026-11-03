/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#include "data/data_screenshot_mode.h"

#include "base/random.h"
#include "lang/lang_keys.h"

#include <QtCore/QHashFunctions>

namespace Data::Screenshot {
namespace {

[[nodiscard]] uint64 Mix(uint64 value) {
	value ^= value >> 30;
	value *= 0xbf58476d1ce4e5b9ULL;
	value ^= value >> 27;
	value *= 0x94d049bb133111ebULL;
	return value ^ (value >> 31);
}

[[nodiscard]] uint64 ProcessSalt() {
	static const auto result = base::RandomValue<uint64>();
	return result;
}

[[nodiscard]] QString FormatName(PeerKind kind, const QString &token) {
	switch (kind) {
	case PeerKind::User:
		return tr::lng_screenshot_pseudonym_user(tr::now, lt_token, token);
	case PeerKind::Bot:
		return tr::lng_screenshot_pseudonym_bot(tr::now, lt_token, token);
	case PeerKind::Channel:
		return tr::lng_screenshot_pseudonym_channel(tr::now, lt_token, token);
	case PeerKind::Forum:
		return tr::lng_screenshot_pseudonym_forum(tr::now, lt_token, token);
	case PeerKind::Group:
		return tr::lng_screenshot_pseudonym_group(tr::now, lt_token, token);
	case PeerKind::Chat:
		return tr::lng_screenshot_pseudonym_chat(tr::now, lt_token, token);
	}
	Unexpected("Unknown screenshot pseudonym kind.");
}

} // namespace

QString PseudonymToken(uint64 sessionSalt, uint64 identity) {
	const auto mixed = Mix(identity ^ Mix(sessionSalt));
	auto result = QString::number(mixed, 36).toUpper();
	return result.rightJustified(13, QChar('0'));
}

QString PeerName(
		PeerKind kind,
		const void *sessionIdentity,
		uint64 identity) {
	const auto sessionSalt = ProcessSalt()
		^ uint64(reinterpret_cast<uintptr_t>(sessionIdentity));
	return FormatName(kind, PseudonymToken(sessionSalt, identity));
}

QString HiddenPeerName(PeerKind kind, QStringView identity) {
	const auto value = uint64(qHash(identity, size_t(ProcessSalt())));
	return FormatName(kind, PseudonymToken(ProcessSalt(), value));
}

} // namespace Data::Screenshot
