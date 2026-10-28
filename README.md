# moregram – Based on [Telegram Desktop](https://github.com/telegramdesktop/tdesktop)

The source code is published under GPLv3 with OpenSSL exception, the license is available [here][license].

[![Preview of moregram][preview_image]][preview_image_url]

## Project Goal

moregram 是 [Telegram Desktop](https://github.com/telegramdesktop/tdesktop) 的強化分支：跟上游開發線保持零落後，並加上增強設定頁、JSON viewer、翻譯目標語系可選等官方沒有的功能，同時在 Linux 上以系統函式庫建置、直接出可供 pacman 使用的套件。

## Roadmap

跟著上游 `dev` 走，有需要就加自己的功能。

## [Features](features.md)

## Supported systems

Windows 7 and above

Linux 64 bit

macOS > 10.12 and above

最新版本與建置產物都在 [Release](https://github.com/Capricornus007/moregram/releases) 頁面。

## Localization

想翻譯就直接動手：譯文檔放在 `Telegram/Resources/langs/localization/`（檔名用語系 ID，例如 `en.json`），
翻好的語言檔提 Pull Request 到 [Capricornus007/moregram](https://github.com/Capricornus007/moregram) 即可。

**專案的翻譯範本見 [`Telegram/Resources/langs/localization/en.json`](Telegram/Resources/langs/localization/en.json)。**

You can find a language ID on Telegram's log.txt

For example: `[2022.04.23 10:37:45] Current Language pack ID: de, Base ID: `

Then your language translation filename is `de.json` or something like that.

***Note: Ignore base ID(base ID translation - Work in progress)***

## Third-party

* Qt 6 ([LGPL](http://doc.qt.io/qt-6/lgpl.html)) and Qt 5.15 ([LGPL](http://doc.qt.io/qt-5/lgpl.html)) slightly patched
* OpenSSL 3.2.1 ([Apache License 2.0](https://openssl-library.org/source/license/apache-license-2.0.txt))
* WebRTC ([New BSD License](https://github.com/desktop-app/tg_owt/blob/master/LICENSE))
* zlib ([zlib License](http://www.zlib.net/zlib_license.html))
* LZMA SDK 9.20 ([public domain](http://www.7-zip.org/sdk.html))
* liblzma ([public domain](http://tukaani.org/xz/))
* Google Breakpad ([License](https://chromium.googlesource.com/breakpad/breakpad/+/master/LICENSE))
* Google Crashpad ([Apache License 2.0](https://chromium.googlesource.com/crashpad/crashpad/+/master/LICENSE))
* GYP ([BSD License](https://github.com/bnoordhuis/gyp/blob/master/LICENSE))
* Ninja ([Apache License 2.0](https://github.com/ninja-build/ninja/blob/master/COPYING))
* OpenAL Soft ([LGPL](https://github.com/kcat/openal-soft/blob/master/COPYING))
* Opus codec ([BSD License](http://www.opus-codec.org/license/))
* FFmpeg ([LGPL](https://www.ffmpeg.org/legal.html))
* Guideline Support Library ([MIT License](https://github.com/Microsoft/GSL/blob/master/LICENSE))
* Range-v3 ([Boost License](https://github.com/ericniebler/range-v3/blob/master/LICENSE.txt))
* Open Sans font ([Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html))
* Vazirmatn font ([SIL Open Font License 1.1](https://github.com/rastikerdar/vazirmatn/blob/master/OFL.txt))
* Emoji alpha codes ([MIT License](https://github.com/emojione/emojione/blob/master/extras/alpha-codes/LICENSE.md))
* xxHash ([BSD License](https://github.com/Cyan4973/xxHash/blob/dev/LICENSE))
* QR Code generator ([MIT License](https://github.com/nayuki/QR-Code-generator#license))
* CMake ([New BSD License](https://github.com/Kitware/CMake/blob/master/Copyright.txt))
* Hunspell ([LGPL](https://github.com/hunspell/hunspell/blob/master/COPYING.LESSER))
* Ada ([Apache License 2.0](https://github.com/ada-url/ada/blob/main/LICENSE-APACHE))
* zxcvbn-c ([MIT License](https://github.com/tsyrogit/zxcvbn-c/blob/master/LICENSE.txt))
* FrequencyWords by Hermit Dave ([Creative Commons Attribution-ShareAlike 4.0](https://creativecommons.org/licenses/by-sa/4.0/))

## Build instructions

* [Windows (32-bit and 64-bit)][win]
* [macOS][mac]
* [GNU/Linux using Docker][linux]

## Links

* 本倉：[Capricornus007/moregram](https://github.com/Capricornus007/moregram)
* 上游：[telegramdesktop/tdesktop](https://github.com/telegramdesktop/tdesktop)

[//]: # (LINKS)
[license]: LICENSE
[win]: docs/building-win.md
[mac]: docs/building-mac.md
[linux]: docs/building-linux.md
[preview_image]: https://github.com/Capricornus007/moregram/blob/dev/docs/assets/preview.png "Preview of moregram Desktop"
[preview_image_url]: https://raw.githubusercontent.com/Capricornus007/moregram/dev/docs/assets/preview.png
