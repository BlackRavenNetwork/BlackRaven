// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_GUICONSTANTS_H
#define RAVEN_QT_GUICONSTANTS_H

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 250;

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* RavenGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

static const bool DEFAULT_SPLASHSCREEN = true;

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080; border: 1px solid lightgray; padding: 0px;"
#define STYLE_VALID "border: 1px solid lightgray; padding: 0px;"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 0, 0)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)
/* Transaction list -- TX status decoration - open until date */
#define COLOR_TX_STATUS_OPENUNTILDATE QColor(64, 64, 255)
/* Transaction list -- TX status decoration - danger, tx needs attention */
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
/* Transaction list -- TX status decoration - default color */
#define COLOR_BLACK QColor(0, 0, 0)
/* Widget Background color - default color */
#define COLOR_WHITE QColor(255, 255, 255)

#define COLOR_WALLETFRAME_SHADOW QColor(0,0,0,71)

/* Color of labels */
#define COLOR_LABELS QColor("#818cf8")

/** LIGHT MODE */
/* Background color */
#define COLOR_BACKGROUND_LIGHT QColor("#f8fafc")
/* Primary accent (gold) */
#define COLOR_DARK_ORANGE QColor("#c9a227")
/* Secondary accent */
#define COLOR_LIGHT_ORANGE QColor("#e5cf6a")
/* Deep slate / header */
#define COLOR_DARK_BLUE QColor("#0f172a")
/* Mid slate */
#define COLOR_LIGHT_BLUE QColor("#475569")
#define COLOR_ASSET_TEXT QColor(255, 255, 255)
#define COLOR_SHADOW_LIGHT QColor("#e2e8f0")
#define COLOR_TOOLBAR_NOT_SELECTED_TEXT QColor("#94a3b8")
#define COLOR_TOOLBAR_SELECTED_TEXT COLOR_WHITE
/* Send entries background color */
#define COLOR_SENDENTRIES_BACKGROUND QColor("#fbfbfe")


/** DARK MODE */
#define COLOR_WIDGET_BACKGROUND_DARK QColor("#0f1419")
#define COLOR_SHADOW_DARK QColor("#030712")
#define COLOR_LIGHT_BLUE_DARK QColor("#1e293b")
#define COLOR_DARK_BLUE_DARK QColor("#020617")
#define COLOR_PRICING_WIDGET QColor("#111827")
#define COLOR_ADMIN_CARD_DARK COLOR_BLACK
#define COLOR_REGULAR_CARD_DARK_BLUE_DARK_MODE QColor("#0c1424")
#define COLOR_REGULAR_CARD_LIGHT_BLUE_DARK_MODE QColor("#162238")
#define COLOR_TOOLBAR_NOT_SELECTED_TEXT_DARK_MODE QColor("#64748b")
#define COLOR_TOOLBAR_SELECTED_TEXT_DARK_MODE QColor("#e2e8f0")
/* Send entries background color dark mode */
#define COLOR_SENDENTRIES_BACKGROUND_DARK QColor("#1c2535")


#define STRING_LABEL_COLOR "color: #818cf8"
#define STRING_LABEL_COLOR_WARNING "color: #FF8080"








/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* QRCodeDialog -- size of exported QR Code image */
#define QR_IMAGE_SIZE 300

/* Number of frames in spinner animation */
#define SPINNER_FRAMES 36

#define QAPP_ORG_NAME "BlackRaven"
#define QAPP_ORG_DOMAIN "blackraven.network"
#define QAPP_APP_NAME_DEFAULT "BlackRaven-Qt"
#define QAPP_APP_NAME_TESTNET "BlackRaven-Qt-testnet"

/* Default third party browser urls */
#define DEFAULT_THIRD_PARTY_BROWSERS "https://api.blackraven.network/tx/%s|https://blackraven.network/tx/%s"

/* Default IPFS viewer */
#define DEFAULT_IPFS_VIEWER "https://ipfs.io/ipfs/%s"

#endif // RAVEN_QT_GUICONSTANTS_H
