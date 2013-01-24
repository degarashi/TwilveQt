#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QDir>
#include <QMovie>
#include <QMouseEvent>
#include "twqtl.h"
#include "twqtweet.h"
#include "twqcell.h"
#include "twqNet.h"
#include "common.h"
#include "dlg_user.h"
#include "twqMain.h"
#include "json_kwd.h"
#include "twqresource.h"

const TwqTL::ActionDef TwqTL::cs_action[] = {
	{MENU::USER, QT_TR_NOOP("&show user info"), ""},
	{MENU::USER, QT_TR_NOOP("send &reply"), ""},

	{MENU::TWEET, QT_TR_NOOP("&reply"), ""},
	{MENU::TWEET, QT_TR_NOOP("&favorite"), ""},
	{MENU::TWEET, QT_TR_NOOP("&unfavorite"), ""},
	{MENU::TWEET, QT_TR_NOOP("&official RT"), ""},
	{MENU::TWEET, QT_TR_NOOP("&cancel official RT"), ""},
	{MENU::TWEET, QT_TR_NOOP("unofficial &RT"), ""},
	{MENU::TWEET, QT_TR_NOOP("unofficial &QT"), ""},
	{MENU::TWEET, QT_TR_NOOP("remove"), ""},

	{MENU::TL, QT_TR_NOOP("&refresh"), ""},
	{MENU::TL, QT_TR_NOOP("&close"), ""}
};

const int TwqTL::CELLWIDTH = 420,
			TwqTL::GAPWIDTH = 10,
			TwqTL::TITLEWIDTH = 24;

TwqTL::TwqTL(const QString &tlname, QWidget *p): QWidget(p),
	_bCellChanged(false), _minID(INVALID_TweetID), _maxID(INVALID_TweetID),
	_gapID(0), _state(STATE::IDLE),
	_selTwID(INVALID_TweetID), _selUserID(INVALID_UserID),
	_nMaxCell(10), _bBtnC(false), _spVoid(new int)
{
	_initAction();
	// Checker / Decorator の初期化
	// (本当は定数なので毎回やる必要は無いが、とりあえず)
	// TwqResourceから色やアイコンを取得
	addMod(CellMod("rt", 0x1000, new CcRT, new CdColor(sgRes.getColor(TwqResource::COLOR::RETWEET))));
	addMod(CellMod("myfav",
				   0x2000,
				   new CcMyFav,
				   new CdIcon(sgRes.getIcon(TwqResource::ICON::FAVORITE).pixmap(16,16,QIcon::Normal, QIcon::On), "myfav")));
	addMod(CellMod("myrt",
				   0x3000,
				   new CcMyRT,
				   new CdIcon(sgRes.getIcon(TwqResource::ICON::RETWEET).pixmap(16,16,QIcon::Normal, QIcon::On), "myrt")));
	addMod(CellMod("select",
				   0x4000,
				   new CcSelect,
				   new CdColor(sgRes.getColor(TwqResource::COLOR::SELECT_TEXT))));

	// construct TitleBar
	_titleBar.lbTitle = new QLabel(tlname);
	_titleBar.lbTitle->setFixedHeight(TITLEWIDTH);
	_titleBar.lbTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	_titleBar.lbTitle->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	_titleBar.lbTitle->setBackgroundRole(QPalette::Dark);
	_titleBar.lbTitle->setAutoFillBackground(true);

	// :close button
	_titleBar.btnClose = new QPushButton("X");
	_titleBar.btnClose->setFixedWidth(TITLEWIDTH);

	QHBoxLayout* loTitle = new QHBoxLayout;
	loTitle->addWidget(_titleBar.lbTitle, 1);
	loTitle->addWidget(_titleBar.btnClose, 0);
	loTitle->setMargin(0);
	loTitle->setContentsMargins(0,0,0,0);

	_titleBar.wTitle = new QWidget;
	_titleBar.wTitle->setLayout(loTitle);

	// inner ScrollArea
	_wTw = new QWidget;
	_wTw->setLayout(new QVBoxLayout);
	_wTw->layout()->setMargin(0);
	_wTw->layout()->setContentsMargins(0,0,0,0);
	_wTw->layout()->setSpacing(0);
	// construct TweetArea
	_scTw = new QScrollArea;
	_scTw->setWidget(_wTw);
	_scTw->setFixedWidth(CELLWIDTH + _scTw->verticalScrollBar()->sizeHint().width() + 4);
	_scTw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	_contBtn = new QPushButton(tr("-- read more --"));

	QVBoxLayout* loTop = new QVBoxLayout;
	loTop->setMargin(0);
	loTop->addWidget(_titleBar.wTitle, 0);
	loTop->addWidget(_scTw, 1, Qt::AlignHCenter);
	setLayout(loTop);
	loTop->setMargin(0);
	loTop->setSizeConstraint(QLayout::SetMinimumSize);

	connect(_titleBar.btnClose, SIGNAL(clicked()), this, SLOT(close()));
	QScrollBar* sc = _scTw->verticalScrollBar();
	connect(sc, SIGNAL(valueChanged(int)), this, SLOT(onSlide(int)));
	connect(_contBtn, SIGNAL(clicked()), this, SLOT(slot_readNextPage()));

	// gap line
	_gap = new QFrame(this);
	_gap->hide();
	_gap->setFrameShadow(QFrame::Sunken);
	_gap->setFrameShape(QFrame::WinPanel);
	_gap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
	_gap->setFixedHeight(GAPWIDTH);
	QPalette pl = _gap->palette();
	pl.setColor(QPalette::Base, QColor(0,0,0));
	_gap->setPalette(pl);

	// loading icon
	_icLoading = new QLabel(_titleBar.lbTitle);

	// :locate file path
	auto& m = sgRes.getMovie(TwqResource::MOVIE::LOADING);
	_icLoading->setMovie(&m);
	m.start();

	int icH = _icLoading->sizeHint().height();
	_icLoading->move(TITLEWIDTH/2, TITLEWIDTH/2 - icH/2);
	_icLoading->hide();
	_titleBar.lbTitle->show();
	_titleBar.wTitle->show();
}
void TwqTL::addMod(CellMod&& mod) {
	_mods.push_back(std::forward<CellMod>(mod));
	// 優先度でソート
	int nM = _mods.size();
	for(int i=1 ; i<nM ; i++) {
		int iCur = i;
		int cur = i-1;
		while(cur>=0) {
			if(_mods[cur].getPrio() <= _mods[iCur].getPrio())
				break;
			std::swap(_mods[cur], _mods[iCur]);
			iCur = cur;
			--cur;
		}
	}
}
void TwqTL::onSlide(int pos) {
	QAbstractSlider* s =_scTw->verticalScrollBar();
	// 一番下までスクロールしたかチェック
//	if(pos == s->maximum())
//		_showBtnC(true);
}

void TwqTL::setShowClose(bool bCB) {
	if(bCB)
		_titleBar.btnClose->show();
	else
		_titleBar.btnClose->hide();
}

void TwqTL::setGapID(TweetID id) {
	_gapID = id;
}
void TwqTL::endLoading(const QString &msg) {
//    Q_ASSERT(_state == ST_LOADING);
	_state = STATE::IDLE;
	_icLoading->hide();

	// ステータスバーに表示
	emit status(msg);
	repaint();
}

void TwqTL::setTLName(const QString& name) {
	_titleBar.lbTitle->setText(name);
	_name = name;
}
void TwqTL::setTitle(const QString& title) {
	_title = title;
}
const QString& TwqTL::title() const {
	return _title;
}
const QString& TwqTL::tlname() const {
	return _name;
}

void TwqTL::close() {
	this->deleteLater();
}

bool TwqTL::_beginLoading() {
	// Idleの時だけ受け付ける
	if(_state == STATE::IDLE) {
		_state = STATE::IDLE;
		// show loading icon on title bar
		_icLoading->show();
		return true;
	}
	return false;
}

void TwqTL::slot_readNextPage() {
	if(_beginLoading())
		readNextPage();
}
void TwqTL::slot_refreshPage() {
	if(_beginLoading())
		refreshPage();
}

#define DEF_ACT(ENF, SLNAME) connect(_action[(int)ACTION::ENF], SIGNAL(triggered()), this, SLOT(SLNAME()));
void TwqTL::_initAction() {
	for(int i=0 ; i<(int)MENU::NUM_MENU ; i++)
		_menu[i] = new QMenu();

	// アクションの作成
	for(int i=0 ; i<(int)ACTION::NUM_ACTION ; i++) {
		auto& def = cs_action[i];
		auto* act = _action[i] = new QAction(tr(def.name), this);
		_menu[(int)def.menu]->addAction(act);
		if(def.shortcut[0] != '\0')
			act->setShortcut(QKeySequence(def.shortcut));
	}

	// シグナル & スロット接続設定
	DEF_ACT(TWEET_REPLY, tweetReply)
	DEF_ACT(TWEET_FAV, tweetFav)
	DEF_ACT(TWEET_UNFAV, tweetUnfav)
	DEF_ACT(TWEET_OFCRT, tweetOfficialRT)
	DEF_ACT(TWEET_UNOFCRT, tweetUnOfficialRT)
	DEF_ACT(TWEET_NONOFCRT, tweetNonOfficialRT)
	DEF_ACT(TWEET_NONOFCQT, tweetNonOfficialQT)
	DEF_ACT(TWEET_REMOVE, tweetRemove)

	DEF_ACT(USER_INFO, showUserInfo)
	DEF_ACT(USER_REPLY, sendUserReply)

	DEF_ACT(TL_REFRESH, slot_refreshPage)
	DEF_ACT(TL_CLOSE, close)
}
#undef DEF_ACT

void TwqTL::paintEvent(QPaintEvent *e) {
	if(_bCellChanged) {
		_bCellChanged = false;

		// ツイート選択を解除
		_selectTweet(INVALID_TweetID);

		bool bc = _bBtnC;
		showContinueButton(false);

		// 縦幅計算
		int h_sum = 0;
		for(auto& cp : _cell)
			h_sum += cp.second->boxLayout()->minimumHeightForWidth(CELLWIDTH);
		_wTw->setFixedSize(CELLWIDTH, std::max(0,h_sum)+GAPWIDTH);

		// 一度すべてのcellを解除した後にソート済みのcellを登録
		QVBoxLayout* lo = qobject_cast<QVBoxLayout*>(_wTw->layout());
		QLayoutItem* child;
		while((child=lo->takeAt(0)) != nullptr);
		Q_ASSERT(lo->count() == 0);

		if(_cell.empty())
			showContinueButton(false);
		else {
			// 新しいツイートから順に配置していく
			auto itr = _cell.end();
			while(itr != _cell.begin()) {
				--itr;
				// 適切な位置でギャップラインを入れる
				if(_gapID >= (*itr).second->id()) {
					_gapID = std::numeric_limits<decltype(_gapID)>::min();
					lo->addWidget(_gap);
					_gap->show();
				}

				lo->addWidget(itr->second);
			}
			itr = _cell.end();
			--itr;
			_maxID = itr->first;
			_minID = _cell.begin()->first;

			showContinueButton(bc);
		}
	}
	QWidget::paintEvent(e);
}
// TLを一番下までスクロールした時に「続きを読むボタン」を表示
void TwqTL::showContinueButton(bool bShow) {
	if(bShow == _bBtnC)
		return;

	QLayout* l = _wTw->layout();
	if(bShow) {
		_contBtn->show();
		l->addWidget(_contBtn);
		l->setAlignment(_contBtn, Qt::AlignCenter);
	} else {
		_contBtn->hide();
		l->removeWidget(_contBtn);
	}
	_bBtnC = bShow;
}

void TwqTL::cellRemoved(TwqCell *cell) {
	auto itr = _cell.find(cell->id());
	if(itr != _cell.end())
		_cell.erase(itr);
}

void TwqTL::addCell(TweetID id) {
	// すでに同じIDを持っていたら何もしない
	if(_cell.count(id) != 0)
		return;

	TwqCell* cell = new TwqCell(id);
	_cell.insert(std::make_pair(id, cell));
	_wTw->layout()->addWidget(cell);
	cell->applyMods(*this, _mods);
	connect(cell, SIGNAL(clicked(TweetID,bool)), this, SLOT(tweetClicked(TweetID,bool)));
	connect(cell, SIGNAL(userClicked(UserID,bool)), this, SLOT(userClicked(UserID,bool)));

	// ID範囲の更新
	_minID = (_minID != 0) ? std::min(id, _minID) : id;
	_maxID = (_maxID != 0) ? std::max(id, _maxID) : id;

	_bCellChanged = true;
}
std::tuple<TweetID,TweetID> TwqTL::idRange() const {
	return std::make_tuple(_minID, _maxID);
}
int TwqTL::ncell() const {
	return _cell.size();
}
int TwqTL::maxcell() const {
	return _nMaxCell;
}
void TwqTL::_selectTweet(TweetID nextID) {
	if(nextID == _selTwID)
		return;
	auto prevID = _selTwID;
	_selTwID = nextID;
	// set to normal state (previous cell)
	if(prevID != INVALID_TweetID) {
		_cell.at(prevID)->applyMods(*this, _mods);
	}
	// set as highlighten (next cell)
	if(nextID != INVALID_TweetID) {
		_cell.at(nextID)->applyMods(*this, _mods);
	}
}
void TwqTL::mousePressEvent(QMouseEvent *e) {
	// ツイートの選択解除
	_selectTweet(INVALID_TweetID);
	if(e->button() == Qt::RightButton) {
		// TLメニューを出す
		_menu[(int)MENU::TL]->exec(QCursor::pos());
	}
}
TweetID TwqTL::getSelectTweet() const {
	return _selTwID;
}

// ----------------------- <アクション群> -----------------------
void TwqTL::tweetReply() {
	auto tid = _selTwID;
	if(tid != INVALID_TweetID)
		emit userOp(USEROP::TWEET_REPLY, tid);
}

void TwqTL::_tweetOp(USEROP op) {
	if(_selTwID != INVALID_TweetID)
		emit userOp(op, _selTwID);
}
void TwqTL::sendUserReply() {
	if(_selUserID != INVALID_UserID)
		emit userOp(USEROP::USER_REPLY, _selUserID);
}
void TwqTL::tweetFav() {
	_tweetOp(USEROP::TWEET_FAVORITE);
}
void TwqTL::tweetUnfav() {
	_tweetOp(USEROP::TWEET_UNFAVORITE);
}
void TwqTL::tweetOfficialRT() {
	_tweetOp(USEROP::TWEET_OFCRT);
}
void TwqTL::tweetUnOfficialRT() {
	_tweetOp(USEROP::TWEET_CANCEL_OFCRT);
}
void TwqTL::tweetNonOfficialRT() {
	_tweetOp(USEROP::TWEET_NOFCRT);
}
void TwqTL::tweetNonOfficialQT() {
	_tweetOp(USEROP::TWEET_QT);
}
void TwqTL::tweetRemove() {
	_tweetOp(USEROP::TWEET_REMOVE);
}
void TwqTL::tweetClicked(TweetID id, bool bR) {
	// どちらのボタンでもツイートを選択状態にする
	_selectTweet(id);
	if(bR) {
		// 即利用できる情報と遅延情報の二段構え
		auto* t = sgTweet.immGetInfo<TwqTweet::P_Home>(id);
		int mask = 0,
			imyfav = 0,
			imyrt = 0;
		// RT/Fav状態の確認、アクション切り替え
		if(t) {
			mask = ~0;
			// MyFavチェック
			imyfav = t->isFavorited() ? 1 : 0;
			// MyRTチェック
			imyrt = t->isReTweeted() ? 1 : 0;
		} else {
			sgTweet.getInfo<TwqTweet::P_Home>(id, _spVoid, [this](const TwqTweet::P_Home& p) {
				int imyfav, imyrt;
				// MyFavチェック
				imyfav = p.isFavorited() ? 1 : 0;
				// MyRTチェック
				imyrt = p.isReTweeted() ? 1 : 0;

				_action[(int)ACTION::TWEET_FAV]->setEnabled(imyfav^1);
				_action[(int)ACTION::TWEET_UNFAV]->setEnabled(imyfav);
				_action[(int)ACTION::TWEET_OFCRT]->setEnabled(imyrt^1);
				_action[(int)ACTION::TWEET_UNOFCRT]->setEnabled(imyrt);
			});
		}
		_action[(int)ACTION::TWEET_FAV]->setEnabled((imyfav^1) & mask);
		_action[(int)ACTION::TWEET_UNFAV]->setEnabled(imyfav & mask);
		_action[(int)ACTION::TWEET_OFCRT]->setEnabled((imyrt^1) & mask);
		_action[(int)ACTION::TWEET_UNOFCRT]->setEnabled(imyrt & mask);

		// 自分のツイートならば削除アクションを有効にする
		auto* tu = sgTweet.immGetInfo<TwqTweet::P_User>(id);
		bool bRem = tu && tu->userID() == sgNet.getMyID();
		_action[(int)ACTION::TWEET_REMOVE]->setEnabled(bRem);
		// メニューを出す
		_menu[(int)MENU::TWEET]->exec(QCursor::pos());
	}
}
void TwqTL::userClicked(UserID id, bool bR) {
	if(bR) {
		// メニューを出す
		_selUserID = id;
		_menu[(int)MENU::USER]->exec(QCursor::pos());
	}
}
void TwqTL::titleClicked(bool bR) {

}
void TwqTL::_procCell(TWQEVENT e, const QVariant& value) {
	// 情報の蓄積と、削除/再描画
	TweetID id = value.toULongLong();
	Q_ASSERT(id != 0);

	auto itr = _cell.find(id);
	if(e == TWQEVENT::TWEET_ADDED) {
		if(itr == _cell.end()) {
			addCell(id);
			_bCellChanged = true;
			repaint();
		}
	} else {
		if(itr != _cell.end()) {
			if(e == TWQEVENT::TWEET_REMOVED) {
				if(id == _selTwID)
					_selectTweet(INVALID_TweetID);
				delete itr->second;
				_cell.erase(itr);
			} else {
				// Tweetエントリを修正してからもう一度Decoに通す
				itr->second->applyMods(*this, _mods);
			}
			_bCellChanged = true;
			repaint();
		}
	}
}
void TwqTL::twqEvent(TWQEVENT e, const QVariant& value) {
	auto typ = DetectEventType(e);
	if(typ == TWQEVENTTYPE::TWEET)
		_procCell(e, value);
}

void TwqTL::showUserInfo() {
	// show userinfo dialog
	DlgUser* dlg = new DlgUser;
	dlg->show();
	dlg->showUser(_selUserID);
	connect(dlg, SIGNAL(userOp(USEROP,QVariant)), this, SIGNAL(userOp(USEROP,QVariant)));
	connect(&sgMain, SIGNAL(twqEvent(TWQEVENT,QVariant)), dlg, SLOT(twqEvent(TWQEVENT,QVariant)));
}
