#ifndef TWQCELL_H
#define TWQCELL_H

#include <QString>
#include <QLabel>
#include <cstdint>
#include <memory>
#include "common.h"
class QFrame;
class QPixmap;
class QMouseEvent;
class QHBoxLayout;
class QVBoxLayout;

// QLabelにクリック検知を追加
class CLabel : public QLabel {
	Q_OBJECT
	uint64_t	_id;	//!< TweetID 又は UserID

	protected:
		void mousePressEvent(QMouseEvent *e);
	signals:
		void clickedUser(UserID id, bool bR);
		void clickedTweet(TweetID id, bool bR);
	public:
		explicit CLabel(uint64_t id);
};

class TwqCell;
class TwqTL;
//! TwqCellの装飾チェック
class IFCellChecker {
	public:
		virtual ~IFCellChecker() {}
		virtual bool check(const TwqTL& tl, TweetID id) const = 0;
};
//! TwqCellの装飾
class IFCellDecorator {
	public:
		virtual ~IFCellDecorator() {}
		virtual void modify(const TwqTL& tl, TwqCell& cell) const = 0;
		virtual void unmodify(const TwqTL& tl, TwqCell& cell) const = 0;
};
//! CheckFlag + Decorator
class CellMod {
	typedef std::unique_ptr<IFCellChecker>	UPCheck;
	typedef std::unique_ptr<IFCellDecorator> UPDeco;
	QString		_name;
	uint32_t	_priority;
	UPCheck		_checker;
	UPDeco		_deco;

	public:
		CellMod(CellMod&& c);
		//! chkとdecoの所有権を持つ
		CellMod(const QString& name, uint32_t prio, IFCellChecker* chk, IFCellDecorator* deco);
		//! 条件判定の後に装飾
		void checkAndModify(const TwqTL& tl, TwqCell& cell) const;
		void unModify(const TwqTL& tl, TwqCell& cell) const;
		void swap(CellMod& c) noexcept;
		uint32_t getPrio() const;
};
namespace std {
	template <>
	inline void swap<CellMod>(CellMod& c0, CellMod& c1) noexcept {
		c0.swap(c1);
	}
}

#define DEF_CCHECK(Name) class Name : public IFCellChecker { \
	public: \
	bool check(const TwqTL& tl, TweetID id) const override; \
};
// HomeTLとSearchTLでは判定方法が異なる
//! 他人のRT
DEF_CCHECK(CcRT)
//! 自分のFav
DEF_CCHECK(CcMyFav)
//! 自分のRT
DEF_CCHECK(CcMyRT)
//! 選択中かどうか
DEF_CCHECK(CcSelect)

//! バックグラウンドを単色で装飾
class CdColor : public IFCellDecorator {
	QColor  _colorBG,
			_colorText;
	public:
		CdColor(QColor colBG=QColor(), QColor colText=QColor()): _colorBG(colBG), _colorText(colText) {}
		void modify(const TwqTL& tl, TwqCell& cell) const override;
		void unmodify(const TwqTL& tl, TwqCell& cell) const override;
};
//! バーにアイコンを置く装飾
class CdIcon : public IFCellDecorator {
	private:
		QPixmap _icon;
		QString _name;
	public:
		CdIcon(const QPixmap& p, const QString& name);
		void modify(const TwqTL& tl, TwqCell& cell) const override;
		void unmodify(const TwqTL& tl, TwqCell& cell) const override;
};

//! TwqTweetを表示するクラス
class TwqCell : public QFrame {
	Q_OBJECT

	QHBoxLayout *_loIconTw,
				*_loSub,
				*_loTray;
	QVBoxLayout* _loMainSub;

	TweetID	_twID;
	// user icon
	CLabel *_lbIcon;
	QLabel *_lbTweet,
			*_lbScreen_name, *_lbName,
			*_lbSource, *_lbDate;
	// for Fav/RT icons
	QFrame* _frIcon;
	SPVoid	_spVoid;

	protected:
		void mousePressEvent(QMouseEvent *e);
	signals:
		void clicked(TweetID twID, bool bR);
		void userClicked(UserID uid, bool bR);
	public:
		explicit TwqCell(TweetID twID, QWidget* parent=nullptr);

		void setTweet(TweetID twID);
		TweetID id() const;
		int cellHeight(int w) const;
		QVBoxLayout* boxLayout();
		QHBoxLayout* iconTray();

		void applyMods(const TwqTL& tl, const std::vector<CellMod>& mods);
};

#endif // TWQCELL_H
