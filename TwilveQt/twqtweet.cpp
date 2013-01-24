#include "twqtweet.h"
#include "json_kwd.h"
#include "twqNet.h"

const TwqTweet::RA_Full TwqTweet::cs_raFull;
const TwqTweet::ReqAct* TwqTweet::cs_RAList[] = {&cs_raFull};

TwqTweet::TwqTweet(QObject *parent): QObject(parent) {}
TwqTweet::Tweet::Tweet(TweetID id): ResourceBase(id) {
	_v.reTweetID = INVALID_TweetID;
}
TwqTweet::Tweet::Tweet(Tweet&& t): ResourceBase(t.id) {
	_v = t._v;
}
TweetID TwqTweet::Tweet::getID() const {
	return id;
}

// ------------ TwqTweet::P_Base ------------
void TwqTweet::P_Base::receiveInfo(const QJsonObject& jobj) {
	if(jobj.find(JSONKWD::TWEET::IdStr) != jobj.end()) {
		_v.text = jobj.find(JSONKWD::TWEET::Text).value().toString();
		QString str = jobj.find(JSONKWD::TWEET::Source).value().toString();
		_v.source = ConvertHESC(str, 1);

		// SearchとHomeの場合で形式が違う
		QString s = jobj.find(JSONKWD::TWEET::Created_at).value().toString();
		// from Home
		_v.created = ReadDT_Home(s);
		if(!_v.created.isValid()) {
			// from Search
			_v.created = ReadDT_Search(s);
		}
		haveF |= Flag;
	} else
		Q_ASSERT(false);
}
QDateTime TwqTweet::P_Base::createdAt() const {
	return _v.created;
}
QString TwqTweet::P_Base::text() const {
	return _v.text;
}
QString TwqTweet::P_Base::source() const {
	return _v.source;
}

// ------------ TwqTweet::P_User ------------
void TwqTweet::P_User::receiveInfo(const QJsonObject& jobj) {
	// from TwqTweet
	auto itr = jobj.find(JSONKWD::TWEET::FromUserId);
	if(itr != jobj.end()) {
		// from Search
		_v.userID = JtoInt(jobj, JSONKWD::TWEET::FromUserIdStr);
		_v.toUserID = JtoInt(jobj, JSONKWD::TWEET::ToUserIDStr);
		haveF |= Flag;
	} else {
		// from Home
		auto itrU = jobj.find(JSONKWD::TWEET::User);
		if(itrU != jobj.end()) {
			_v.userID = JtoInt(itrU.value().toObject(), JSONKWD::USER::IdStr);
			_v.toUserID = JtoInt(jobj, JSONKWD::TWEET::InReplyToUserID);
			haveF |= Flag;
		}
	}
}
UserID TwqTweet::P_User::userID() const {
	return _v.userID;
}
UserID TwqTweet::P_User::toUserID() const {
	return _v.toUserID;
}

// ------------ TwqTweet::P_Home ------------
void TwqTweet::P_Home::receiveInfo(const QJsonObject& jobj) {
	// Retweeted持ってればOK
	if(jobj.find(JSONKWD::TWEET::ReTweeted) != jobj.end()) {
		// retweetedがtrueかretweeted_statusを持っていればtrue
		_v.bRetweeted = jobj.find(JSONKWD::TWEET::ReTweeted).value().toBool();
		_v.bFavorited = jobj.find(JSONKWD::TWEET::Favorited).value().toBool();
		_v.rtCount = JtoInt(jobj, JSONKWD::TWEET::ReTweet_count);
		_v.toTweetID = JtoInt(jobj, JSONKWD::TWEET::InReplyToStatusId);
		haveF |= Flag;
	}
}
TweetID TwqTweet::P_Home::toTweetID() const {
	return _v.toTweetID;
}
bool TwqTweet::P_Home::isReTweeted() const {
	return _v.bRetweeted;
}
bool TwqTweet::P_Home::isFavorited() const {
	return _v.bFavorited;
}
int TwqTweet::P_Home::rtCount() const {
	return _v.rtCount;
}

// ------------ TwqTweet::P_RT ------------
void TwqTweet::P_RT::receiveInfo(const QJsonObject& jobj) {
	if(jobj.contains(JSONKWD::TWEET::ReTweeted)) {
		auto itr = jobj.find(JSONKWD::TWEET::ReTweeted_stat);
		if(itr != jobj.end()) {
			// retweeted_statusの時は中に入ってる情報が元で、外がRTIDになっている
			// userのIDが自分であればRTフラグをON
			auto jtu = jobj.find(JSONKWD::TWEET::User).value().toObject();
			_v.bRetweeted = JtoInt(jtu, JSONKWD::USER::Id) == sgNet.getMyID();
			_v.reTweetID = getID();
			auto jtw = itr.value().toObject();
			_v.srcTweetID = JtoInt(jtw, JSONKWD::TWEET::IdStr);
			haveF |= Flag;
		} else if((itr=jobj.find(JSONKWD::TWEET::CurrentUserRetweet)) != jobj.end()) {
			_v.bRetweeted = true;
			_v.reTweetID = JtoInt(itr.value().toObject(), JSONKWD::TWEET::IdStr);
			_v.srcTweetID = INVALID_TweetID;
			haveF |= Flag;
		}
	}
}
TweetID TwqTweet::P_RT::reTweetID() const {
	return _v.reTweetID;
}
TweetID TwqTweet::P_RT::srcID() const {
	return _v.srcTweetID;
}
bool TwqTweet::P_RT::isRT() const {
	return reTweetID() == INVALID_TweetID;
}

// ------------ TwqTweet::P_Full ------------
TwqTweet::RA_Full::RA_Full(): ReqAct(0, P_User::Flag|P_Home::Flag|P_RT::Flag|P_Full::Flag, 1) {}
void TwqTweet::P_Full::receiveInfo(const QJsonObject& jobj) {
	if(jobj.find("entities") != jobj.end())
		haveF |= Flag;
}
void TwqTweet::RA_Full::doIt(Tweet& t) const {
	TweetID tid = t.id;
	ParamMap pm;
	pm[TWAPIKWD::TW_SHOW::IncludeMyRetweet] = "true";
	sgNet.restAPI(sgTweet.GetCB<TwqTweet::P_Full>(tid), TWAPI::TW_SHOW, pm, tid);
}
