#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QLabel>
#include "common.h"
#include "twqNet.h"

class TwqTL;
namespace Ui {
	class MainWindow;
}
#define sgMain TwqMain::_ref()
class TwqMain : public QMainWindow, public Singleton<TwqMain> {
	Q_OBJECT
	std::shared_ptr<Ui::MainWindow>	_ui;

	enum class MENU {
		APPLICATION,
		COLUMN,
		ACTION,
		NUM_MENU
	};
	QMenu* _menu[(int)MENU::NUM_MENU];
	const static char* cs_menu[(int)MENU::NUM_MENU];

	struct ActionDef {
		MENU		menuID;		//!< メニュー項目ID
		const char	*name,		//!< アクションの名前
					*shortcut;	//!< ショートカット文字列
	};

	enum class ACTION {
		APP_AUTH,
		APP_LIMIT,
		APP_QUIT,
		COLUMN_SEARCH,
		COLUMN_HOME,
		COLUMN_MENTIONS,
		TWEET,
		NUM_ACTION
	};
	const static ActionDef cs_action[(int)ACTION::NUM_ACTION];
	QAction* _action[sizeof(cs_action)/sizeof(cs_action[0])];

	//! API制限表示用
	std::unique_ptr<QLabel>		_lbAPILimit;
	TwqNet* _pNet;

	SPVoid	_spVoid;

	void _addTL(TwqTL* tl, bool bRefl);
	//! ツイートウィンドウを開く
	void _openTweetWindow(const QString& pre, const QString& post);

	public slots:
		//! from OAuth 認証の結果
		void authResult(bool bSuccess, const QString& err);
		//! from OAuth トークンの状態が変化した際に呼ぶ
		void stateChanged(TwqNet::TOKEN state);
		void quit();
		void updateAPILimit(const TwqNet::APILimit& api);
		void _openTweetWindow();
		//! ステータスウィンドウに文字列を表示
		void showStatus(const QString& stat);
		//! 様々なWidgetからの、ユーザーに対するオペレーション
		void userOp(USEROP op, const QVariant& value);
		void onNetworkError(uint64_t ucode, const QString& msg);
		void showAPILimit();
		// ---- TL関連 ----
		void addTLSearch();
		void addTLHome();
		void addTLMentions();
	signals:
		void twqEvent(TWQEVENT e, const QVariant& value);
	public:
		explicit TwqMain(QWidget *parent = 0);
};

#endif // MAINWINDOW_H
