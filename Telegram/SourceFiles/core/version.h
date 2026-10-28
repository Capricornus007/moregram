/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/const_string.h"

#define TDESKTOP_REQUESTED_ALPHA_VERSION (0ULL)

#ifdef TDESKTOP_ALLOW_CLOSED_ALPHA
#define TDESKTOP_ALPHA_VERSION TDESKTOP_REQUESTED_ALPHA_VERSION
#else // TDESKTOP_ALLOW_CLOSED_ALPHA
#define TDESKTOP_ALPHA_VERSION (0ULL)
#endif // TDESKTOP_ALLOW_CLOSED_ALPHA

// used in Updater.cpp and Setup.iss for Windows
// moregram：AppId 換成自行產生的 GUID（對照 gram64/master 的 64Gram 自有 AppId），
// AppName/AppFile 是品牌顯示名的來源（托盤、通知、g_set_application_name、預設下載資料夾）。
constexpr auto AppId = "{914E9916-3DE1-4E74-9F8A-8729B91B1714}"_cs;
constexpr auto AppNameOld = "Telegram Win (Unofficial)"_cs;
constexpr auto AppName = "moregram"_cs;
constexpr auto AppFile = "moregram"_cs;
constexpr auto AppVersion = 7003003;
constexpr auto AppVersionStr = "7.3.3";
constexpr auto AppBetaVersion = false;
constexpr auto AppAlphaVersion = TDESKTOP_ALPHA_VERSION;
constexpr auto UpstreamVersion = "7.3.3";