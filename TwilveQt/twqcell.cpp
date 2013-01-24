#include <QMouseEvent>
#include <QJsonObject>
#include <QHBoxLayout>
#include <QLayoutItem>
#include "twqcell.h"
#include "json_kwd.h"
#include "twqtweet.h"
#include "twquser.h"
#include "twqNet.h"
#include "twqtl.h"

// ------------------- CLabel -------------------
CLabel::CLabel(uint64_t id): _id(id) {}
void CLabel::mousePressEvent(QMouseEvent *e) {
	bool bR = e->button() == Qt::RightButton;
	emit clickedUser(_id, bR);
	emit clickedTweet(_id, bR);
}

// ------------------- CellMod -------------------
CellMod::CellMod(CellMod&& c) {
	swap(c);
}
CellMod::CellMod(const QString& name, uint32_t prio, IFCellChecker* chk, IFCellDecorator* decorator):
	_name(name), _priority(prio), _checker(chk), _deco(decorator) {}
void CellMod::checkAndModify(const TwqTL& tl, TwqCell& cell) const {
	if(_checker->check(tl, cell.id()))
		_deco->modify(tl, cell);
}
void CellMod::unModify(const TwqTL& tl, TwqCell& cell) const {
	_deco->unmodify(tl, cell);
}
void CellMod::swap(CellMod& c) noexcept {
	std::swap(_name, c._name);
	std::swap(_priority, c._priority);
	std::swap(_checker, c._checker);
	std::swap(_deco, c._deco);
}
uint32_t CellMod::getPrio() const {
	return _priority;
}
// ------------------- CellCheck -------------------
bool CcMyRT::check(const TwqTL& tl, TweetID id) const {
	// 自分がRTしたか？
	auto* p = sgTweet.immGetInfo<TwqTweet::P_Home>(id);
	return p && p->isReTweeted();
}
bool CcRT::check(const TwqTL& tl, TweetID id) const {
	// 誰かにRTされているか？ かつ自分がRTしていないか
	auto* p = sgTweet.immGetInfo<TwqTweet::P_Home>(id);
	if(p) {
		auto nRT = p->rtCount();
		if(p->isReTweeted())
			--nRT;
		return nRT > 0;
	}
	return false;
}
bool CcMyFav::check(const TwqTL& tl, TweetID id) const {
	// 自分がFavしたか？
	auto* p = sgTweet.immGetInfo<TwqTweet::P_Home>(id);
	return p && p->isFavorited();
}
bool CcSelect::check(const TwqTL& tl, TweetID id) const {
	// TLで選択されているか？
	return id == tl.getSelectTweet();
}

// ------------------- CellDecorator -------------------
void CdColor::modify(const TwqTL& tl, TwqCell& cell) const {
	// 親からデフォルト色を取ってくる
	const QPalette& pp = cell.parentWidget()->palette();

	QPalette p = cell.palette();
	p.setColor(QPalette::Window,
			   (_colorBG.isValid()) ? _colorBG : pp.color(QPalette::Window));
	p.setColor(QPalette::WindowText,
			   (_colorText.isValid()) ? _colorText : pp.color(QPalette::WindowText));
	cell.setPalette(p);
}
void CdColor::unmodify(const TwqTL& tl, TwqCell& cell) const {
	// 親のカラーを適用
	cell.setPalette(cell.parentWidget()->palette());
}

CdIcon::CdIcon(const QPixmap& p, const QString& name): _icon(p), _name(name) {}
void CdIcon::modify(const TwqTL& tl, TwqCell& cell) const {
	// アイコンをその都度生成
	QLabel* lb = new QLabel;
	lb->setPixmap(_icon);
	lb->setScaledContents(true);
	lb->setFixedSize(16,16);
	lb->setObjectName(_name);

	// アイコンがダブる事は無いものと仮定
	QHBoxLayout* l = cell.iconTray();
	l->addWidget(lb);
}
void CdIcon::unmodify(const TwqTL& tl, TwqCell& cell) const {
	QLayout* l = cell.iconTray();
	int n = l->count();
	for(int i=0 ; i<n ; i++) {
		QWidget* w = l->itemAt(i)->widget();
		if(w) {
			if(w->objectName() == _name) {
				w->hide();
				l->removeWidget(w);
				return;
			}
		}
	}
}

// ------------------- TwqCell -------------------
TwqCell::TwqCell(TweetID twID, QWidget* parent): QFrame(parent, Qt::Widget), _spVoid(new int) {
	_twID = twID;

	// init cell layout
	_loIconTw = new QHBoxLayout;    // Icon : Tweet => Main
	_loMainSub = new QVBoxLayout;   // Main : Sub => Cell
	_loSub = new QHBoxLayout;       // UserName : OtherInfo... => Sub

	// :Tweet
	_lbTweet = new QLabel;
	_lbTweet->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	_lbTweet->setWordWrap(true);
	_lbTweet->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
	// :Icon
	auto* t = sgTweet.immGetInfo<TwqTweet::P_User>(twID);
	_lbIcon = new CLabel(t->userID());
	_lbIcon->setFixedSize(64,64);
	_loIconTw->addWidget(_lbIcon, 0);
	_loIconTw->addWidget(_lbTweet, 1);
	_loMainSub->setMargin(0);
	_loMainSub->setSpacing(0);

	setAutoFillBackground(true);
	setFrameShadow(QFrame::Raised);
	setFrameShape(QFrame::WinPanel);

	// :UserName
	_lbName = new QLabel;
	QFont font(_lbName->font());
	font.setBold(true);
	font.setPointSize(font.pointSize()-1);
	_lbName->setFont(font);

	// :Source
	_lbSource = new QLabel;
	// :Date&Time
	_lbDate = new QLabel;
	// :IconTray
	_frIcon = new QFrame;
	_frIcon->setFixedHeight(16);

	_loTray = new QHBoxLayout;
	_frIcon->setLayout(_loTray);
	_loTray->setMargin(0);
	_loTray->setSpacing(0);

	_loMainSub->addLayout(_loIconTw, 4);
	_loMainSub->addLayout(_loSub, 0);

	_loSub->addWidget(_lbName, 0, Qt::AlignLeft);
	_loSub->addWidget(_frIcon, 0, Qt::AlignHCenter);
	_loSub->addWidget(_lbDate, 0, Qt::AlignRight);
	_loSub->addWidget(_lbSource, 0, Qt::AlignRight);

	setLayout(_loMainSub);

	setTweet(twID);
	connect(_lbIcon, SIGNAL(clickedUser(UserID, bool)), this, SIGNAL(userClicked(UserID, bool)));
}
void TwqCell::setTweet(TweetID twID) {
	sgTweet.getInfo<TwqTweet::P_User>(twID, _spVoid, [this](const TwqTweet::P_User& pu) {
		// user nameの設定
		sgUser.getInfo<TwqUser::P_Name>(pu.userID(), _spVoid, [this](const TwqUser::P_Name& pn) {
			_lbName->setText(pn.name());
		});

		// Tweet
		_lbTweet->setText(pu.text());
		// Source
		_lbSource->setTextFormat(Qt::RichText);
		_lbSource->setText(QString(tr("from %1")).arg(pu.source()));
		// Data
		QDateTime dt = pu.createdAt();
		_lbDate->setText(dt.toString("ddd hh:mm:ss"));

		// Pictureインタフェースを取得してアイコン設定
		sgUser.getInfo<TwqUser::P_Picture>(pu.userID(), _spVoid, [this](const TwqUser::P_Picture& pn) {
			_lbIcon->setPixmap(*pn.profileIcon());
		});
	});
}
QVBoxLayout* TwqCell::boxLayout() {
	return _loMainSub;
}
QHBoxLayout* TwqCell::iconTray() {
	return _loTray;
}
void TwqCell::applyMods(const TwqTL& tl, const std::vector<CellMod>& mods) {
	for(auto& itr : mods)
		itr.unModify(tl, *this);
	for(auto& itr : mods)
		itr.checkAndModify(tl, *this);
}
TweetID TwqCell::id() const {
	return _twID;
}
void TwqCell::mousePressEvent(QMouseEvent *e) {
	// クリックされたことを知らせる為にシグナルを出す
	emit clicked(_twID, e->button()==Qt::RightButton);
}
