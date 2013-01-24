#include <QUrl>
#include <QDebug>
#include "twquser.h"
#include "json_kwd.h"
#include "twqNet.h"
#include <QJsonArray>

const TwqUser::RA_Prop TwqUser::cs_raProp;
const TwqUser::RA_Picture TwqUser::cs_raPic;
const TwqUser::ReqAct* TwqUser::cs_RAList[CTRAct::size] = {&cs_raProp, &cs_raPic};

// ------------ TwqUser::RA_Prop ------------
TwqUser::RA_Prop::RA_Prop(): ReqAct(0, P_Name::Flag|P_Prop::Flag|P_PictureURL::Flag|P_Follow::Flag|P_Followed::Flag, 3) {}
void TwqUser::RA_Prop::doIt(User& u) const {
	UserID uid = u.getID();
	// 実際にTwitterAPIで問い合わせ
	ParamMap pm;
	pm.insert(TWAPIKWD::US_LOOKUP::UserId, QString("%1").arg(uid));
	sgNet.restAPI(sgUser.GetCB<TwqUser::P_Prop>(uid), TWAPI::US_LOOKUP, pm, QString(), uid);
}

// ------------ TwqUser::RA_Picture ------------
TwqUser::RA_Picture::RA_Picture(): ReqAct(P_PictureURL::Flag, P_Picture::Flag, 1) {}
void TwqUser::RA_Picture::doIt(User& u) const {
	auto& us = reinterpret_cast<P_PictureURL&>(u);
	auto urlstr = us.profileIconURL();
	UserID uid = u.id;
	// Httpで画像を取得
	sgNet.getImage([uid](QPixmap* pm) {
		auto* u = sgUser.immRefInfo<P_Picture>(uid);
		if(u) {
			u->receiveInfo(pm);
			u->receiveReply(P_Picture::Flag);
		}
	}, QUrl(urlstr));
}

UserID TwqUser::User::getID() const {
	return id;
}
// ------------ TwqUser::P_Name ------------
void TwqUser::P_Name::receiveInfo(const QJsonObject& jobj) {
	// from Search
	if(jobj.contains(JSONKWD::TWEET::FromUserName)) {
		_v.name = jobj.find(JSONKWD::TWEET::FromUserName).value().toString();		// 表示名
		_v.scrName = jobj.find(JSONKWD::TWEET::FromUser).value().toString();		// アカ名
		haveF |= Flag;
	} else if(jobj.contains(JSONKWD::USER::Name)) {
		// from Home
		_v.name = jobj.find(JSONKWD::USER::Name).value().toString();
		_v.scrName = jobj.find(JSONKWD::USER::Screen_name).value().toString();
		haveF |= Flag;
	}
}
const QString& TwqUser::P_Name::name() const {
	return _v.name;
}
const QString& TwqUser::P_Name::screenName() const {
	return _v.scrName;
}

// ------------ TwqUser::P_Prop ------------
const QString& TwqUser::P_Prop::description() const {
	return _v.description;
}
int TwqUser::P_Prop::nTweet() const {
	return _v.nTweet;
}
int TwqUser::P_Prop::nFollow() const {
	return _v.nFollow;
}
int TwqUser::P_Prop::nFollower() const {
	return _v.nFollower;
}
void TwqUser::P_Prop::receiveInfo(const QJsonObject& jobj) {
	// Descriptionを持っていればOK
	if(jobj.contains(JSONKWD::USER::Description)) {
		_v.description = jobj.find(JSONKWD::USER::Description).value().toString();
		_v.nTweet = JtoInt(jobj, JSONKWD::USER::Statuses_count);
		_v.nFollow = JtoInt(jobj, JSONKWD::USER::Friends_count);
		_v.nFollower = JtoInt(jobj, JSONKWD::USER::Followers_count);
		haveF |= Flag;
	}
}
// ------------ TwqUser::P_PictureURL ------------
void TwqUser::P_PictureURL::receiveInfo(const QJsonObject& jobj) {
	// profile_url項があればOK
	auto itr = jobj.find(JSONKWD::USER::Profile_url);
	if(itr != jobj.end()) {
		_v.profURL = itr.value().toString();
		haveF |= Flag;
	}
}
QUrl TwqUser::P_PictureURL::profileIconURL() const {
	return _v.profURL;
}

// ------------ TwqUser::P_Picture ------------
QPixmap* TwqUser::P_Picture::profileIcon() const {
	return _pixIcon.get();
}
void TwqUser::P_Picture::receiveInfo(const QJsonObject& jobj) {
	// JSONから得られる物では無い
}
void TwqUser::P_Picture::receiveInfo(QPixmap* pm) {
	_pixIcon.reset(pm);
}

// ------------ TwqUser::P_Follow ------------
void TwqUser::P_Follow::receiveInfo(const QJsonObject& jobj) {
	auto itr = jobj.find(JSONKWD::USER::Following);
	if(itr != jobj.end()) {
		_v.bFollowing = itr.value().toBool();
		haveF |= Flag;
	}
	itr = jobj.find("connections");
	if(itr != jobj.end()) {
		// friendships/lookup タイプ
		_v.bFollowing = itr.value().toObject().contains("following");
		haveF |= Flag;
	}
}
bool TwqUser::P_Follow::bFollowing() const {
	return _v.bFollowing;
}

// ------------ TwqUser::P_Followed ------------
void TwqUser::P_Followed::receiveInfo(const QJsonObject& jobj) {
	auto itr = jobj.find("connections");
	if(itr != jobj.end()) {
		_v.bFollowed = itr.value().toObject().contains("followed_by");
		haveF |= Flag;
	}
}
bool TwqUser::P_Followed::bFollowed() const {
	return _v.bFollowed;
}

// ------------ TwqUser::User ------------
TwqUser::User::User(User&& u): User(u.id) {
	_pixIcon.swap(u._pixIcon);
	haveF = u.haveF;
	reqAct.swap(u.reqAct);
	reqQ.swap(u.reqQ);
	bReq = u.bReq;

	_v = u._v;
}
TwqUser::User::User(UserID id): ResourceBase(id) {
	haveF = 0;
	bReq = false;
}
// ------------ TwqUser ------------
TwqUser::TwqUser(QObject* parent): QObject(parent) {
	connect(&sgNet, SIGNAL(onNetworkError(uint64_t,QString)), this, SLOT(onNetworkError(uint64_t,QString)));
}
//! RestAPIコールに失敗したらとりあえず、そのユーザー情報を消しておく
void TwqUser::onNetworkError(uint64_t id, const QString& msg) {
	_base.removeInfo(id);
	qDebug() << tr("user query failed(userID=%1)").arg(id);
}
