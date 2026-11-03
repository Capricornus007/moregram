/*
This file is part of moregram Desktop,
the unofficial app based on Telegram Desktop.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/dev/LEGAL
*/
#include "api/api_copy_resend.h"

#include "api/api_sending.h"
#include "base/call_delayed.h"
#include "apiwrap.h"
#include "base/flat_map.h"
#include "base/random.h"
#include "core/application.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_media_types.h"
#include "data/data_saved_sublist.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "data/data_thread.h"
#include "history/history.h"
#include "history/history_item.h"
#include "iv/iv_instance.h"
#include "iv/iv_rich_message_serializer.h"
#include "iv/iv_rich_page.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "storage/localimageloader.h"
#include "storage/storage_account.h"
#include "ui/chat/attach/attach_prepare.h"
#include "ui/layers/show.h"
#include "ui/toast/toast.h"
#include "ui/image/image.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/labels.h"
#include "styles/style_boxes.h"
#include "styles/style_chat.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace Api::CopyResend {
namespace {

constexpr auto kMaximumActiveJobs = 2;
constexpr auto kCleanupDelay = crl::time(24 * 60 * 60 * 1000);

class Job;

[[nodiscard]] auto &ActiveJobs() {
	static auto result = base::flat_map<uint64, std::shared_ptr<Job>>();
	return result;
}

[[nodiscard]] TextWithTags OriginalText(not_null<HistoryItem*> item) {
	return {
		item->originalText().text,
		TextUtilities::ConvertEntitiesToTextTags(item->originalText().entities),
	};
}

[[nodiscard]] ContentKind KindFor(not_null<HistoryItem*> item) {
	if (item->richPage()) {
		return ContentKind::RichMessage;
	}
	const auto media = item->media();
	if (!media) {
		return item->emptyText() ? ContentKind::Unsupported : ContentKind::Text;
	} else if (media->photo()) {
		return ContentKind::Photo;
	} else if (const auto document = media->document()) {
		return document->sticker()
			? ContentKind::Sticker
			: document->isVoiceMessage()
			? ContentKind::Voice
			: document->isVideoMessage()
			? ContentKind::RoundVideo
			: (document->isVideoFile() || document->isGifv())
			? ContentKind::Video
			: ContentKind::Document;
	}
	return item->emptyText() ? ContentKind::Unsupported : ContentKind::Text;
}

[[nodiscard]] SourceDescriptor Describe(not_null<HistoryItem*> item) {
	const auto kind = KindFor(item);
	const auto media = item->media();
	const auto document = media ? media->document() : nullptr;
	const auto photo = media ? media->photo() : nullptr;
	return {
		.kind = kind,
		.groupId = item->groupId().value,
		.bytes = (kind == ContentKind::Sticker || kind == ContentKind::RichMessage)
			? 0
			: document
			? document->size
			: photo
			? std::max(photo->imageByteSize(Data::PhotoSize::Large), 0)
			: 0,
		.protectedContent = !item->allowsForward(),
		.service = item->isService(),
		.expiring = media && (media->ttlSeconds() > 0),
	};
}

[[nodiscard]] QString ExtensionFor(
		ContentKind kind,
		not_null<HistoryItem*> item) {
	if (kind == ContentKind::Photo) {
		return u".jpg"_q;
	}
	const auto document = item->media()->document();
	const auto suffix = QFileInfo(document->filename()).suffix();
	if (!suffix.isEmpty()) {
		return QChar('.') + suffix;
	}
	return (kind == ContentKind::Voice)
		? u".ogg"_q
		: (kind == ContentKind::Video || kind == ContentKind::RoundVideo)
		? u".mp4"_q
		: u".bin"_q;
}

[[nodiscard]] SendMediaType SendType(ContentKind kind) {
	switch (kind) {
	case ContentKind::Photo:
	case ContentKind::Video:
		return SendMediaType::Photo;
	case ContentKind::Voice:
		return SendMediaType::Audio;
	case ContentKind::RoundVideo:
		return SendMediaType::Round;
	case ContentKind::Document:
		return SendMediaType::File;
	default:
		return SendMediaType::File;
	}
}

void RemoveDirectoryAsync(QString path) {
	crl::async([path = std::move(path)] {
		QDir(path).removeRecursively();
	});
}

void PruneInactiveDirectories(
		const QString &root,
		const QStringList &preserved) {
	const auto directory = QDir(root);
	for (const auto &entry : directory.entryInfoList(
			QDir::Dirs | QDir::NoDotAndDotDot)) {
		if (!preserved.contains(entry.absoluteFilePath())) {
			QDir(entry.absoluteFilePath()).removeRecursively();
		}
	}
}

class Job final : public std::enable_shared_from_this<Job> {
public:
	explicit Job(Request request)
	: _request(std::move(request))
	, _id(base::RandomValue<uint64>())
	, _progress(std::make_shared<rpl::variable<QString>>()) {
	}

	[[nodiscard]] uint64 id() const {
		return _id;
	}

	void start() {
		_request.show->hideLayer();
		showProgress();
		setProgress(tr::lng_copy_resend_preparing(tr::now));
		const auto temp = _request.session->local().tempDirectory();
		_root = QDir(temp).filePath(u"moregram-copy-resend"_q);
		_jobDirectory = QDir(_root).filePath(QString::number(_id, 16));
		auto preserved = QStringList();
		preserved.reserve(ActiveJobs().size());
		for (const auto &entry : ActiveJobs()) {
			if (!entry.second->_jobDirectory.isEmpty()) {
				preserved.push_back(entry.second->_jobDirectory);
			}
		}
		const auto weak = weak_from_this();
		crl::async([weak, root = _root, job = _jobDirectory, preserved] {
			PruneInactiveDirectories(root, preserved);
			const auto ready = QDir().mkpath(job);
			crl::on_main([weak, ready] {
				if (const auto strong = weak.lock()) {
					if (ready) {
						strong->prepareNext();
					} else {
						strong->fail(Error::TemporaryStorageLimit);
					}
				}
			});
		});
	}

private:
	struct PreparedSource {
		FullMsgId id;
		ContentKind kind = ContentKind::Unsupported;
		uint64 groupId = 0;
		QString path;
		std::vector<Ui::PreparedFile> copies;
		std::shared_ptr<const Iv::RichPage> richPage;
	};

	void showProgress() {
		const auto weak = weak_from_this();
		const auto progress = _progress;
		_progressBox = _request.show->show(Box<Ui::GenericBox>([=](
				not_null<Ui::GenericBox*> box) {
			box->setTitle(tr::lng_copy_resend_progress_title());
			box->setCloseByEscape(false);
			box->setCloseByOutsideClick(false);
			box->addRow(object_ptr<Ui::FlatLabel>(
				box,
				progress->value(),
				st::boxLabel));
			box->addButton(tr::lng_cancel(), [weak] {
				if (const auto strong = weak.lock()) {
					strong->cancel();
				}
			});
		}));
	}

	void setProgress(QString text) {
		_progress->force_assign(std::move(text));
	}

	void cancel() {
		if (_finished) {
			return;
		}
		_cancelled = true;
		_waitingLifetime.destroy();
		if (_startedDownload) {
			if (_waitingDocument) {
				_waitingDocument->cancel();
			} else if (_waitingPhoto) {
				_waitingPhoto->cancel();
			}
		}
		finish(false, tr::lng_copy_resend_cancelled(tr::now));
	}

	void prepareNext() {
		if (_cancelled || _finished) {
			return;
		} else if (_prepareIndex >= _request.ids.size()) {
			sendAll();
			return;
		}
		const auto id = _request.ids[_prepareIndex];
		const auto item = _request.session->data().message(id);
		if (!item) {
			fail(Error::UnsupportedMedia);
			return;
		}
		const auto kind = KindFor(item);
		setProgress(tr::lng_copy_resend_progress(
			tr::now,
			lt_done,
			QString::number(_prepareIndex),
			lt_total,
			QString::number(_request.ids.size())));
		if (kind == ContentKind::Text || kind == ContentKind::Sticker) {
			appendPrepared({
				.id = id,
				.kind = kind,
				.groupId = item->groupId().value,
			});
		} else if (kind == ContentKind::RichMessage) {
			prepareRich(item);
		} else if (kind == ContentKind::Photo) {
			preparePhoto(item);
		} else if (kind == ContentKind::Document
			|| kind == ContentKind::Video
			|| kind == ContentKind::Voice
			|| kind == ContentKind::RoundVideo) {
			prepareDocument(item, kind);
		} else {
			fail(Error::UnsupportedMedia);
		}
	}

	void appendPrepared(PreparedSource prepared) {
		_prepared.push_back(std::move(prepared));
		++_prepareIndex;
		_waitingLifetime.destroy();
		_waitingDocument = nullptr;
		_waitingPhoto = nullptr;
		_startedDownload = false;
		prepareNext();
	}

	void prepareRich(not_null<HistoryItem*> item) {
		const auto id = item->fullId();
		const auto groupId = item->groupId().value;
		const auto weak = weak_from_this();
		Core::App().iv().resolveRichMessage(
			_request.session,
			item,
			[weak, id, groupId](std::shared_ptr<const Iv::RichPage> page) {
				if (const auto strong = weak.lock()) {
					if (!page) {
						strong->fail(Error::UnsupportedMedia);
						return;
					}
					strong->appendPrepared({
						.id = id,
						.kind = ContentKind::RichMessage,
						.groupId = groupId,
						.richPage = std::move(page),
					});
				}
			});
	}

	[[nodiscard]] QString pathFor(
			FullMsgId id,
			ContentKind kind,
			not_null<HistoryItem*> item) const {
		return QDir(_jobDirectory).filePath(
			QString::number(id.peer.value)
			+ QChar('_')
			+ QString::number(id.msg.bare)
			+ ExtensionFor(kind, item));
	}

	void preparePhoto(not_null<HistoryItem*> item) {
		const auto id = item->fullId();
		const auto photo = item->media()->photo();
		const auto media = photo->createMediaView();
		_waitingPhoto = photo;
		const auto complete = [weak = weak_from_this(), id, media] {
			if (const auto strong = weak.lock()) {
				strong->writePhoto(id, media);
			}
		};
		if (media->loaded()) {
			complete();
			return;
		}
		_waitingLifetime = rpl::single() | rpl::then(
			_request.session->downloaderTaskFinished()
		) | rpl::filter([photo, media] {
			return media->loaded()
				|| (!photo->loading() && photo->failed(Data::PhotoSize::Large));
		}) | rpl::take(1) | rpl::on_next([=] {
			if (media->loaded()) {
				complete();
			} else if (const auto strong = weak_from_this().lock()) {
				strong->fail(Error::UnsupportedMedia);
			}
		});
		_startedDownload = !photo->loading();
		media->wanted(Data::PhotoSize::Large, id);
	}

	void writePhoto(
			FullMsgId id,
			std::shared_ptr<Data::PhotoMedia> media) {
		const auto item = _request.session->data().message(id);
		if (!item) {
			fail(Error::UnsupportedMedia);
			return;
		}
		const auto path = pathFor(id, ContentKind::Photo, item);
		const auto bytes = media->imageBytes(Data::PhotoSize::Large);
		const auto loaded = media->image(Data::PhotoSize::Large);
		const auto image = loaded ? loaded->original() : QImage();
		const auto weak = weak_from_this();
		crl::async([weak, id, path, bytes, image] {
			auto ok = false;
			if (!bytes.isEmpty()) {
				auto file = QFile(path);
				ok = file.open(QIODevice::WriteOnly)
					&& file.write(bytes) == bytes.size()
					&& file.flush();
			} else if (!image.isNull()) {
				ok = image.save(path, "JPG");
			}
			crl::on_main([weak, id, path, ok] {
				if (const auto strong = weak.lock()) {
					if (ok) {
						strong->prepareCopies(id, ContentKind::Photo, path);
					} else {
						strong->fail(Error::UnsupportedMedia);
					}
				}
			});
		});
	}

	void prepareDocument(
			not_null<HistoryItem*> item,
			ContentKind kind) {
		const auto id = item->fullId();
		const auto document = item->media()->document();
		const auto media = document->createMediaView();
		if (const auto loadedPath = document->filepath(true); !loadedPath.isEmpty()) {
			prepareCopies(id, kind, loadedPath);
			return;
		}
		if (const auto bytes = media->bytes(); !bytes.isEmpty()) {
			writeDocumentBytes(id, kind, bytes);
			return;
		}
		const auto path = pathFor(id, kind, item);
		_waitingDocument = document;
		_waitingLifetime = _request.session->downloaderTaskFinished(
		) | rpl::filter([document] {
			return !document->loading();
		}) | rpl::take(1) | rpl::on_next([weak = weak_from_this(), id, kind, path] {
			crl::async([weak, id, kind, path] {
				const auto ready = QFileInfo(path).isFile()
					&& QFileInfo(path).size() > 0;
				crl::on_main([weak, id, kind, path, ready] {
					if (const auto strong = weak.lock()) {
						if (ready) {
							strong->prepareCopies(id, kind, path);
						} else {
							strong->fail(Error::UnsupportedMedia);
						}
					}
				});
			});
		});
		_startedDownload = !document->loading();
		document->save(id, path);
	}

	void writeDocumentBytes(
			FullMsgId id,
			ContentKind kind,
			QByteArray bytes) {
		const auto item = _request.session->data().message(id);
		if (!item) {
			fail(Error::UnsupportedMedia);
			return;
		}
		const auto path = pathFor(id, kind, item);
		const auto weak = weak_from_this();
		crl::async([weak, id, kind, path, bytes = std::move(bytes)] {
			auto file = QFile(path);
			const auto ok = file.open(QIODevice::WriteOnly)
				&& file.write(bytes) == bytes.size()
				&& file.flush();
			crl::on_main([weak, id, kind, path, ok] {
				if (const auto strong = weak.lock()) {
					if (ok) {
						strong->prepareCopies(id, kind, path);
					} else {
						strong->fail(Error::UnsupportedMedia);
					}
				}
			});
		});
	}

	void prepareCopies(FullMsgId id, ContentKind kind, QString path) {
		const auto count = _request.destinations.size();
		const auto weak = weak_from_this();
		const auto item = _request.session->data().message(id);
		const auto displayName = (item && item->media()->document())
			? item->media()->document()->filename()
			: QString();
		crl::async([weak, id, kind, path, displayName, count] {
			auto copies = std::vector<Ui::PreparedFile>();
			copies.reserve(count);
			for (auto i = size_t(0); i != count; ++i) {
				auto file = Ui::PreparedFile(path);
				file.displayName = displayName;
				Storage::PrepareDetails(
					file,
					st::sendMediaPreviewSize,
					PhotoSideLimit());
				copies.push_back(std::move(file));
			}
			crl::on_main([weak, id, kind, path, copies = std::move(copies)]() mutable {
				if (const auto strong = weak.lock()) {
					const auto item = strong->_request.session->data().message(id);
					if (!item) {
						strong->fail(Error::UnsupportedMedia);
						return;
					}
					strong->appendPrepared({
						.id = id,
						.kind = kind,
						.groupId = item->groupId().value,
						.path = path,
						.copies = std::move(copies),
					});
				}
			});
		});
	}

	[[nodiscard]] Api::SendAction actionFor(
			not_null<Data::Thread*> thread,
			not_null<HistoryItem*> source) const {
		auto action = Api::SendAction(thread, _request.options);
		action.clearDraft = false;
		if (GetEnhancedBool("repeater_reply_to_orig_msg")
			&& thread->owningHistory() == source->history()
			&& thread->topicRootId() == source->topicRootId()) {
			action.replyTo = {
				.messageId = source->fullId(),
				.topicRootId = source->topicRootId(),
			};
		}
		return action;
	}

	void sendAll() {
		if (_cancelled || _finished) {
			return;
		}
		for (const auto &source : _prepared) {
			if (source.kind != ContentKind::RichMessage) {
				continue;
			}
			const auto serialized = Iv::SerializeInputRichMessage(
				_request.session,
				*source.richPage,
				Iv::SerializeInputRichMessageMode::FinalSubmit);
			if (serialized.status != Iv::SerializeInputRichMessageStatus::Success
				|| !serialized.value) {
				fail(Error::UnsupportedMedia);
				return;
			}
		}
		setProgress(tr::lng_copy_resend_uploading(tr::now));
		for (auto destinationIndex = size_t(0);
			destinationIndex != _request.destinations.size();
			++destinationIndex) {
			const auto thread = _request.destinations[destinationIndex];
			if (!_request.comment.empty()) {
				auto comment = Api::MessageToSend(
					Api::SendAction(thread, _request.options));
				comment.action.clearDraft = false;
				comment.textWithTags = _request.comment;
				_request.session->api().sendMessage(std::move(comment));
			}
			for (auto i = size_t(0); i != _prepared.size();) {
				auto &source = _prepared[i];
				const auto item = _request.session->data().message(source.id);
				if (!item) {
					++i;
					continue;
				}
				if (source.groupId && source.kind != ContentKind::RichMessage) {
					auto list = Ui::PreparedList();
					const auto groupId = source.groupId;
					auto groupType = SendMediaType::Photo;
					auto j = i;
					for (; j != _prepared.size() && _prepared[j].groupId == groupId; ++j) {
						auto &part = _prepared[j];
						const auto partItem = _request.session->data().message(part.id);
						if (!partItem || part.copies.empty()) {
							continue;
						}
						auto file = std::move(part.copies[destinationIndex]);
						file.caption = OriginalText(partItem);
						if (part.kind == ContentKind::Document) {
							groupType = SendMediaType::File;
						}
						list.files.push_back(std::move(file));
					}
					if (!list.files.empty()) {
						_request.session->api().sendFiles(
							std::move(list),
							groupType,
							std::make_shared<SendingAlbum>(),
							actionFor(thread, item));
					}
					i = j;
					continue;
				}
				sendOne(thread, destinationIndex, source, item);
				++i;
			}
		}
		finish(true, tr::lng_share_done(tr::now));
	}

	void sendOne(
			not_null<Data::Thread*> thread,
			size_t destinationIndex,
			PreparedSource &source,
			not_null<HistoryItem*> item) {
		auto action = actionFor(thread, item);
		if (source.kind == ContentKind::Text) {
			auto message = Api::MessageToSend(std::move(action));
			message.textWithTags = OriginalText(item);
			_request.session->api().sendMessage(std::move(message));
		} else if (source.kind == ContentKind::Sticker) {
			auto message = Api::MessageToSend(std::move(action));
			message.textWithTags = OriginalText(item);
			Api::SendExistingDocument(
				std::move(message),
				item->media()->document());
		} else if (source.kind == ContentKind::RichMessage) {
			const auto serialized = Iv::SerializeInputRichMessage(
				_request.session,
				*source.richPage,
				Iv::SerializeInputRichMessageMode::FinalSubmit);
			if (serialized.status == Iv::SerializeInputRichMessageStatus::Success
				&& serialized.value) {
				_request.session->api().sendRichMessage(
					source.richPage,
					*serialized.value,
					std::move(action));
			}
		} else if (destinationIndex < source.copies.size()) {
			auto list = Ui::PreparedList();
			auto file = std::move(source.copies[destinationIndex]);
			file.caption = OriginalText(item);
			list.files.push_back(std::move(file));
			_request.session->api().sendFiles(
				std::move(list),
				SendType(source.kind),
				nullptr,
				std::move(action));
		}
	}

	void fail(Error error) {
		finish(false, ErrorText(error));
	}

	void finish(bool success, QString text) {
		if (_finished) {
			return;
		}
		_finished = true;
		_waitingLifetime.destroy();
		if (_progressBox) {
			_progressBox->closeBox();
		}
		_request.show->showToast(std::move(text));
		if (success && _request.done) {
			_request.done();
		}
		base::call_delayed(kCleanupDelay, _request.session, [path = _jobDirectory] {
			RemoveDirectoryAsync(path);
		});
		const auto id = _id;
		crl::on_main([id] {
			ActiveJobs().erase(id);
		});
	}

	Request _request;
	const uint64 _id = 0;
	std::shared_ptr<rpl::variable<QString>> _progress;
	base::weak_qptr<Ui::BoxContent> _progressBox;
	QString _root;
	QString _jobDirectory;
	std::vector<PreparedSource> _prepared;
	size_t _prepareIndex = 0;
	DocumentData *_waitingDocument = nullptr;
	PhotoData *_waitingPhoto = nullptr;
	rpl::lifetime _waitingLifetime;
	bool _startedDownload = false;
	bool _cancelled = false;
	bool _finished = false;
};

} // namespace

Plan Inspect(
		not_null<Main::Session*> session,
		const MessageIdsList &ids,
		bool enabled) {
	auto descriptions = std::vector<SourceDescriptor>();
	descriptions.reserve(ids.size());
	for (const auto id : ids) {
		const auto item = session->data().message(id);
		if (!item) {
			descriptions.push_back({ .kind = ContentKind::Unsupported });
		} else {
			descriptions.push_back(Describe(item));
		}
	}
	return BuildPlan(descriptions, enabled);
}

bool CanOffer(
		not_null<Main::Session*> session,
		const MessageIdsList &ids) {
	return bool(Inspect(
		session,
		ids,
		GetEnhancedBool("copy_resend_protected_content")));
}

QString ErrorText(Error error) {
	switch (error) {
	case Error::Disabled:
		return tr::lng_copy_resend_disabled(tr::now);
	case Error::EmptySelection:
		return tr::lng_copy_resend_empty(tr::now);
	case Error::NoProtectedContent:
		return tr::lng_copy_resend_not_protected(tr::now);
	case Error::ServiceMessage:
		return tr::lng_copy_resend_service_unsupported(tr::now);
	case Error::ExpiringMedia:
		return tr::lng_copy_resend_expiring_unsupported(tr::now);
	case Error::UnsupportedMedia:
		return tr::lng_copy_resend_media_unsupported(tr::now);
	case Error::TooManyItems:
		return tr::lng_copy_resend_too_many(tr::now);
	case Error::TemporaryStorageLimit:
		return tr::lng_copy_resend_storage_limit(tr::now);
	case Error::None:
		return QString();
	}
	Unexpected("Unknown copy resend error.");
}

void RepeatInSourceChat(
		not_null<Main::Session*> session,
		FullMsgId id,
		RepeatMode mode,
		PeerData *sendAs) {
	const auto item = session->data().message(id);
	if (!item || item->id <= 0) {
		return;
	}
	const auto history = item->history();
	auto action = Api::SendAction(history, Api::SendOptions{ .sendAs = sendAs });
	action.clearDraft = false;
	if (history->peer->isUser()
		|| history->peer->isChat()
		|| history->peer->isMonoforum()) {
		action.options.sendAs = nullptr;
	}
	if (item->topic()) {
		action.replyTo = {
			.messageId = item->fullId(),
			.topicRootId = item->topicRootId(),
		};
	}
	if (mode == RepeatMode::Copy
		&& GetEnhancedBool("repeater_reply_to_orig_msg")) {
		action.replyTo.messageId = item->fullId();
	}
	if (const auto sublist = item->savedSublist()) {
		action.replyTo.monoforumPeerId = history->peer->isSelf()
			? nullptr
			: sublist->monoforumPeerId();
	}
	if (mode == RepeatMode::Forward) {
		auto resolved = history->resolveForwardDraft(Data::ForwardDraft{
			.ids = MessageIdsList(1, id),
		});
		session->api().forwardMessages(std::move(resolved), action, [] {
			Ui::Toast::Show(tr::lng_share_done(tr::now));
		});
		return;
	}
	if (!item->isService() && !item->emptyText() && !item->media()) {
		auto message = Api::MessageToSend(std::move(action));
		message.textWithTags = OriginalText(item);
		session->api().sendMessage(std::move(message));
	} else if (!item->isService()
		&& item->allowsForward()
		&& item->media()
		&& item->media()->document()
		&& item->media()->document()->sticker()) {
		auto resolved = history->resolveForwardDraft(Data::ForwardDraft{
			.ids = MessageIdsList(1, id),
			.options = Data::ForwardOptions::NoSenderNames,
		});
		session->api().forwardMessages(std::move(resolved), action, [] {
			Ui::Toast::Show(tr::lng_share_done(tr::now));
		});
	}
}

void Start(Request request) {
	const auto plan = Inspect(
		request.session,
		request.ids,
		GetEnhancedBool("copy_resend_protected_content"));
	if (!plan) {
		request.show->showToast(ErrorText(plan.error));
		return;
	} else if (request.destinations.empty()) {
		request.show->showToast(ErrorText(Error::EmptySelection));
		return;
	} else if (ActiveJobs().size() >= kMaximumActiveJobs) {
		request.show->showToast(tr::lng_copy_resend_busy(tr::now));
		return;
	}
	auto job = std::make_shared<Job>(std::move(request));
	ActiveJobs().emplace(job->id(), job);
	job->start();
}

} // namespace Api::CopyResend
