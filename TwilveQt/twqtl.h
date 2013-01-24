#ifndef TWQTL_H
#define TWQTL_H

#include <QWidget>
#include <vector>
#include <map>
#include "twqcell.h"
#include "common.h"

class QPushButton;
class QLabel;
class CellMod;
class QScrollArea;
class TwqCell;
class QFrame;
class QMenu;

//! タイトルバーとコラムビュー
class TwqTL : public QWidget {
	Q_OBJECT
	const static int CELLWIDTH,
					GAPWIDTH,
					TITLEWIDTH;
	//! Cell modifier
	typedef std::vector<CellMod>		ModList;
	ModList		_mods;
	//! デフォルトのCellMod
	/*! CcRT, CcMyRT, CcMyFav の3つ */
	const static CellMod cs_mod[3];

	QString		_title,
				_name;

	// TitleBar
	struct {
			QWidget*		wTitle;
			QLabel*			lbTitle;
			QPushButton*	btnClose;
	}_titleBar;

	// Tweets
	QScrollArea*			_scTw;
	QWidget*				_wTw;
	QPushButton*			_contBtn;
	std::map<TweetID, TwqCell*>	_cell;
	bool					_bCellChanged;
	TweetID					_minID, _maxID;

	// Gap
	QFrame*					_gap;
	TweetID					_gapID;		// 直後に挿入

	// Loading icon
	QLabel*					_icLoading;

	enum class STATE {
		IDLE,			//!< ユーザー入力待機中
		LOADING,		//!< API返答待ち
	};
	STATE					_state;

	enum class ACTION {
		// ユーザーに関するアクション
		USER_INFO,
		USER_REPLY,

		// ツイートに関するアクション
		TWEET_REPLY,
		TWEET_FAV,
		TWEET_UNFAV,
		TWEET_OFCRT,
		TWEET_UNOFCRT,
		TWEET_NONOFCRT,
		TWEET_NONOFCQT,
		TWEET_REMOVE,

		// TLに関するアクション
		TL_REFRESH,
		TL_CLOSE,
		NUM_ACTION
	};

	enum class MENU {
		USER,		//!< to User,
		TWEET,		//!< to Tweet
		TL,			//!< to Timeline
		NUM_MENU
	};
	QMenu*		_menu[(int)MENU::NUM_MENU];

	struct ActionDef {
		MENU		menu;
		const char *name,
					*shortcut;
	};
	const static ActionDef cs_action[(int)ACTION::NUM_ACTION];
	QAction*	_action[(int)ACTION::NUM_ACTION];

	TweetID		_selTwID,		//!< 選択中ツイートID
				_selUserID;		//!< 選択中ユーザーID
	int			_nMaxCell;
	bool		_bBtnC;			//!< [続きを読む]ボタンが表示中か
	SPVoid		_spVoid;		//!< 自身の存在確認用

	void _initAction();
	bool _beginLoading();
	void _tweetOp(USEROP op);

	protected:
		void mousePressEvent(QMouseEvent* e);
		void paintEvent(QPaintEvent* e);
		void showContinueButton(bool b);
		void setGapID(TweetID id);
		void endLoading(const QString& msg);
		//! セルを更新
		/*! \param bRemove trueならセルを削除, falseなら装飾の更新
			選択中のセルなら選択解除する */
		void _procCell(TWQEVENT e, const QVariant& v);
		void _selectTweet(TweetID nextID);
	public slots:
		// 選択中のツイート(TwqCell)に対してアクションを行う
		void tweetReply();
		void tweetFav();
		void tweetUnfav();
		void tweetOfficialRT();
		void tweetUnOfficialRT();
		void tweetNonOfficialRT();
		void tweetNonOfficialQT();
		void tweetRemove();
		void showUserInfo();			//!< ツイート元のユーザー情報を表示
		void sendUserReply();

		// from TwqCell
		void tweetClicked(TweetID id, bool bR);
		void userClicked(UserID id, bool bR);
		// from TitleBar(QLabel)
		void titleClicked(bool bR);

		void cellRemoved(TwqCell* cell);
		void close();
		void onSlide(int pos);

		void twqEvent(TWQEVENT e, const QVariant& value);
		// from UI
		void slot_readNextPage();
		void slot_refreshPage();
	signals:
		void userOp(USEROP op, const QVariant& param);
		void status(const QString& s);
	public:
		explicit TwqTL(const QString& tlname, QWidget* p=0);

		void setTitle(const QString& name);
		void setTLName(const QString& name);
		const QString& title() const;
		const QString& tlname() const;

		void setShowClose(bool bCB);
		void addMod(CellMod&& mod);
		void removeMod(const QString& name);

		//! このTLが保持しているツイートのID範囲
		std::tuple<TweetID,TweetID> idRange() const;

		//! TweetIDに対応したTwqCellを追加
		/*! 既にIDを持っていたら何もしない */
		void addCell(TweetID id);
		void setCellLimit(int nCell);
		int ncell() const;				//!< 現在持っているセルの数
		int maxcell() const;			//!< 保持できるセルの最大数
		TweetID getSelectTweet() const;

		virtual void readNextPage() = 0;
		virtual void refreshPage() = 0;
};

#endif // TWQTL_H
