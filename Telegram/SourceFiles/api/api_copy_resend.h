/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#pragma once

#include "api/api_copy_resend_plan.h"
#include "api/api_common.h"
#include "ui/text/text_entity.h"
#include "data/data_msg_id.h"

class PeerData;


namespace Data {
class Thread;
} // namespace Data

namespace Main {
class Session;
} // namespace Main

namespace Ui {
class Show;
} // namespace Ui

namespace Api::CopyResend {

enum class RepeatMode : uchar {
	Forward,
	Copy,
};

struct Request {
	std::shared_ptr<Ui::Show> show;
	not_null<Main::Session*> session;
	MessageIdsList ids;
	std::vector<not_null<Data::Thread*>> destinations;
	TextWithTags comment;
	Api::SendOptions options;
	Fn<void()> done;
};

[[nodiscard]] Plan Inspect(
	not_null<Main::Session*> session,
	const MessageIdsList &ids,
	bool enabled);
[[nodiscard]] bool CanOffer(
	not_null<Main::Session*> session,
	const MessageIdsList &ids);
[[nodiscard]] QString ErrorText(Error error);
void Start(Request request);
void RepeatInSourceChat(
	not_null<Main::Session*> session,
	FullMsgId id,
	RepeatMode mode,
	PeerData *sendAs);

} // namespace Api::CopyResend
