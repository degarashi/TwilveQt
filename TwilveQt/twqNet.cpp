#include "twqNet.h"
#include <QPixmap>
#include <QNetworkReply>
#include <QDebug>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QJsonObject>
#include "json_kwd.h"
#include <QDesktopServices>
#include "validation.h"
#include "twquser.h"

namespace {
	const static char* c_HESC[][2] = {
		{u8"€", "&euro;"},
		{u8" ", "&nbsp;"},
		{u8"\"", "&quot;"},
		{u8"&", "&amp;"},
		{u8"<", "&lt;"},
		{u8">", "&gt;"}
	};
}
QString ConvertHESC(const QString& str, int dir) {
	QString ret(str);
	for(auto c : c_HESC)
		ret = ret.replace(c[0^dir], c[1^dir]);
	return ret;
}
// search: Sun, 06 May 2012 06:57:33 +0000
// "ddd, dd MMM yyyy HH:mm:ss '''''"
// " +0000"と"ddd, "を削る
QDateTime  ReadDT_Search(const QString& s) {
	QString s0(s);
	s0 = s0.replace(QRegExp("\\s*\\+\\d{4}"), "").replace(QRegExp("^\\w{3}\\,\\s*"), "");
	return QDateTime::fromString(s0, "dd MMM yyyy hh:mm:ss");
}
// home: Thu Jan 05 07:55:59 +0000 2012
// "ddd MMM dd HH:mm:ss ''''' yyyy"
// " +0000"と"ddd "を削る
QDateTime ReadDT_Home(const QString& s) {
	QString s0(s);
	s0 = s0.replace(QRegExp("\\s*\\+\\d{4}"), "").replace(QRegExp("^\\w{3}\\s*"), "");
	return QDateTime::fromString(s0, "MMM dd HH:mm:ss yyyy");
}
namespace {
	const static char
		*CSKEY_FILE = "../conf/cskey",			//!< ConsumerKey&Secretファイル名
		*TOKEN_FILE = "../conf/token";			//!< AccessToken&Secretファイル名
}
#include <QApplication>
#include <stdexcept>
#include <QFile>
#include <QDir>
void TwqNet::_loadCSKey() {
	if(_tokenState == TOKEN::EMPTY) {
		QDir dir(QApplication::applicationDirPath() + '/' + CSKEY_FILE);
		QString cs = _loadFromFile(dir);
		if(!cs.isEmpty()) {
			auto csl = cs.split("|");
			if(csl.size() == 2) {
				_oauth.setCSKey(csl[0], csl[1]);
				_tokenState = TOKEN::KEYS;
				emit stateChanged(getTokenState());
				return;
			}
		}
		throw RuntimeException(tr("consumer-key file has not been loaded"));
	}
	throw LogicException(tr("tried to loading consumer-key file twice"));
}
void TwqNet::_loadToken() {
	if(_tokenState == TOKEN::OK)
		throw LogicException(tr("tried to load token file but alerady has token"));
	try {
		// load TokenID & Secret
		QDir dir(QApplication::applicationDirPath() + '/' + TOKEN_FILE);
		QString tkn = _loadFromFile(dir);
		auto csl = tkn.split("|");
		if(csl.size() == 2) {
			_oauth.setToken(csl[0], csl[1]);
			_tokenState = TOKEN::OK;
			emit stateChanged(getTokenState());
			return;
		}
		throw InvalidFileException(tr("invalid token file"));
	} catch(const RuntimeException& e) {
		// 認証を済ませてない場合はトークンファイルが無い = エラーではない
	}
}
void TwqNet::_saveToken() const {
	if(_tokenState != TOKEN::OK)
		throw LogicException(tr("tried to save token file but has no token"));
	QFile f;
	auto path = QApplication::applicationDirPath() + '/' + TOKEN_FILE;
	f.setFileName(path);
	if(!f.open(QIODevice::WriteOnly))
		throw RuntimeException(tr("failed saving token to file"));

	// 平文で保存はあんまりだから、適当に暗号化する
	auto tkn = _oauth.token().toLatin1();
	tkn.append('|');
	tkn.append(_oauth.secret().toLatin1());
	int len = tkn.length();
	for(int i=0 ; i<len ; i++)
		tkn.data()[i] ^= (i*0xab)^0xc4;
	f.write(tkn);
	f.close();
}

QString TwqNet::_loadFromFile(const QDir &path) {
	QString ret;
	QFile fl(path.absolutePath());
	fl.open(QIODevice::ReadOnly);
	if(!fl.isOpen())
		throw RuntimeException(QString(tr("failed to open file %1")).arg(path.absolutePath()));
	QByteArray ba = fl.readAll();

	// 一応復号処理
	int len = ba.length();
	for(int i=0 ; i<len ; i++)
		ba.data()[i] ^= (i*0xab)^0xc4;

	return ba;
}

TwqNet::TwqNet(QObject* parent): QObject(parent), _tokenState(TOKEN::EMPTY) {
	_myID = std::numeric_limits<decltype(_myID)>::max();
	// NetworkAccessManagerからの応答を受け取る設定
	connect(&_amgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(_netResponse(QNetworkReply*)));
	connect(this, SIGNAL(stateChanged(TwqNet::TOKEN)), this, SLOT(_stateChanged(TwqNet::TOKEN)));
	// CSキーやトークンをファイルから読み込む
	_loadCSKey();
	_loadToken();
}

void TwqNet::_stateChanged(TwqNet::TOKEN state) {
	if(state == TOKEN::OK) {
		// トークンファイルを保存
		_saveToken();
		emit authResult(true, QString());

		// API残り回数を取得してキャッシュ
		restAPI(
			[this](QJsonDocument& jdoc) {
				_apiLimit.readJson(jdoc);
				emit updateAPILimit(_apiLimit);
			},
			TWAPI::AC_LIMIT,
			ParamMap()
		);
		// 自分の情報をUserMgrへ追加, IDを記憶
		restAPI(
			[this](QJsonDocument& jdoc) {
				auto jobj = jdoc.object();
				_myID = JtoInt(jobj, JSONKWD::USER::IdStr);
				// ユーザー情報登録
				sgUser.setInfo(_myID, jobj);
			},
			TWAPI::AC_CREDENTIALS,
			ParamMap()
		);
	}
}

namespace {
	QString c_url_request("https://api.twitter.com/oauth/request_token"),
			c_url_auth("https://api.twitter.com/oauth/authorize"),
			c_url_access("https://api.twitter.com/oauth/access_token");
	const char *c_prop_callback = "callback",
				*c_prop_type = "type",
				*c_prop_uid = "userid";

	typedef std::function<void (QNetworkRequest&)>  ModFunc;
	ModFunc g_nothing = [](QNetworkRequest&){};
	template <class CBT>
	inline QNetworkReply* GetRequest(QNetworkAccessManager& am, const CBT& cb, const QUrl& url, int type, ModFunc proc = g_nothing)
	{
		QNetworkRequest req;
		req.setUrl(url);
		proc(req);

		QNetworkReply* rep = am.get(req);
		rep->setProperty(c_prop_type, type);
		CBT* cbp = new CBT(cb);
		rep->setProperty(c_prop_callback, (intptr_t)cbp);
		return rep;
	}
	template <class CBT>
	inline QNetworkReply* GetAuthRequest(QNetworkAccessManager& am, const OAuth& oauth, const CBT& cb,
					 const QUrl& url, int type, const HSTREntry& addition)
	{
		return GetRequest(am, cb, url, type,
		   [&oauth, &addition](QNetworkRequest& req){
				   oauth.output(&req, "GET", addition);
			});
	}
	template <class CBT>
	inline QNetworkReply* PostRequest(QNetworkAccessManager& am, const CBT& cb, const QUrl& url,
							int type, const HSTREntry& addHead, const QByteArray& body, ModFunc proc = g_nothing)
	{
		QNetworkRequest req;
		req.setUrl(url);
		for(auto itr=addHead.cbegin() ; itr!=addHead.cend() ; itr++)
			req.setRawHeader(itr.key().toUtf8(), itr.value().toUtf8());

		proc(req);          // 認証などの前処理

		QNetworkReply* rep = am.post(req, body);
		rep->setProperty(c_prop_type, type);
		CBT* cbp = new CBT(cb);
		rep->setProperty(c_prop_callback, (intptr_t)cbp);
		return rep;
	}
	template <class CBT>
	inline QNetworkReply* PostAuthRequest(QNetworkAccessManager& am, const OAuth& oauth,
								const CBT& cb, const QUrl& url, int type, const HSTREntry& entHead, const HSTREntry& entBody)
	{
		// application/x-www-form-urlencoded のみの対応とする
		return PostRequest(am, cb, url, type, entHead, entBody.toURLEncoded(false).toUtf8(),
					[&oauth, &entBody](QNetworkRequest& req) {
						oauth.output(&req, "POST", entBody);
					});
	}
	bool ReceiveTokens(QStringList& dst, QNetworkReply* rep) {
		int res = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if(res == 200) {
			QByteArray ba = rep->readAll();
			if(!ba.isEmpty()) {
				QStringList sl = QString(ba).split("&");
				QString s0, s1;
				int count = 0;
				for(const QString& s : sl) {
					QStringList ssl = s.split("=");
					QString ent = ssl[0];
					if(ent == "oauth_token") {
						s0 = ssl[1];
						++count;
					} else if(ent == "oauth_token_secret") {
						s1 = ssl[1];
						++count;
					}
				}
				if(count == 2) {
					dst.clear();
					dst << s0 << s1;
					return true;
				}
			}
		}
		return false;
	}
}
void TwqNet::_openAuthURL() {
	if(_tokenState != TOKEN::AUTH)
		return;
	QUrl url(c_url_auth);
	QUrlQuery urlQ;
	urlQ.addQueryItem("oauth_token", _oauth.token());
	url.setQuery(urlQ);
	QDesktopServices::openUrl(url);

	validation vd;
	QString vid = vd.execValidation();
	if(!vid.isEmpty())
		_getAccToken(vid);
	else {
		_tokenState = TOKEN::KEYS;
		emit authResult(false, tr("user canceled"));
		emit stateChanged(getTokenState());
	}
}
void TwqNet::_getAccToken(const QString &valiID) {
	if(_tokenState != TOKEN::AUTH)
		return;

	ReplyCB cb = [this](QNetworkReply* rep) {
			QStringList qs;
			if(ReceiveTokens(qs, rep)) {
				_oauth.setToken(qs[0], qs[1]);
				this->_tokenState = TOKEN::OK;
			} else {
				emit authResult(false, rep->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
				this->_tokenState = TOKEN::KEYS;
			}
			emit stateChanged(this->getTokenState());
	};

	_oauth.setVeliID(valiID);
	GetAuthRequest(_amgr, _oauth, cb, c_url_access, (int)REQUEST::REPLY, HSTREntry());
	_tokenState = TOKEN::ACCESS;
	emit stateChanged(getTokenState());
}
void TwqNet::beginAuthorization() {
	if(!(_tokenState == TOKEN::REQUEST ||
		_tokenState == TOKEN::KEYS))
	{
		return;
	}
	_tokenState = TOKEN::REQUEST;
	_oauth.setToken("","");

	ReplyCB cb = [this](QNetworkReply* rep) {
		QStringList qs;
		if(ReceiveTokens(qs, rep)) {
			_oauth.setToken(qs[0], qs[1]);
			this->_tokenState = TOKEN::AUTH;
			this->_openAuthURL();
		} else {
			this->_tokenState = TOKEN::KEYS;
			emit authResult(false, rep->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
		}
		emit stateChanged(this->getTokenState());
	};
	GetAuthRequest(_amgr, _oauth, cb, c_url_request, (int)REQUEST::REPLY, HSTREntry());
	_tokenState = TOKEN::REQUEST;
	emit stateChanged(this->getTokenState());
}

void TwqNet::APILimit::reset() {
	memset(this, 0, sizeof(*this));
}
void TwqNet::APILimit::readJson(QJsonDocument& jdoc) {
	reset();

	QJsonObject jobj = jdoc.object();
	remainHits = JtoInt(jobj, JSONKWD::APILIMIT::RemainingHits);
	resetInSec = JtoInt(jobj, JSONKWD::APILIMIT::ResetTimeInSec);
	hourlyLimit = JtoInt(jobj, JSONKWD::APILIMIT::HourlyLimit);
	resetTime = ReadDT_Home(jobj.find(JSONKWD::APILIMIT::ResetTime).value().toString());

	auto itr = jobj.find(JSONKWD::APILIMIT::Photos);
	if(itr != jobj.end()) {
		QJsonObject jop = itr.value().toObject();
		p_remainHits = JtoInt(jop, JSONKWD::APILIMIT::PHOTOS::RemainingHits);
		p_resetInSec = JtoInt(jop, JSONKWD::APILIMIT::PHOTOS::ResetTimeInSec);
		p_dailyLimit = JtoInt(jop, JSONKWD::APILIMIT::PHOTOS::DailyLimit);
		p_resetTime = ReadDT_Home(jop.find(JSONKWD::APILIMIT::PHOTOS::ResetTime).value().toString());
	}
}
void TwqNet::restAPI(RestCB cb, TWAPI::INDEX index, const ParamMap& param, uint64_t num, uint64_t uid) {
	restAPI(cb, index, param, QString("%1").arg(num), uid);
}
void TwqNet::restAPI(RestCB cb, TWAPI::INDEX index, const ParamMap& param, const QString& urlP, uint64_t uid) {
	const TWAPI::TWApiInfo& info = TWAPI::INFO[index];
	QUrl url;
	QUrlQuery urlq;
	url.setHost(info.host());
	url.setPath((urlP.isNull()) ? info.path() : info.path(urlP));
	url.setScheme("https");

	int iAuth = info.requireAuth();
	if(iAuth == 1) {
		// Tokenを持っていれば認証付きで送信
		if(getTokenState() != TOKEN::OK)
			iAuth = 0;
	}

	QNetworkReply* rep;
	if(info.isGET()) {
		// クエリ変数として追加
		for(auto itr=param.cbegin() ; itr!=param.cend() ; itr++)
			urlq.addQueryItem(itr.key(), itr.value());
		url.setQuery(urlq);
		if(iAuth)
			rep = GetAuthRequest(_amgr, _oauth, cb, url, (int)REQUEST::JSON, HSTREntry());
		else
			rep = GetRequest(_amgr, cb, url, (int)REQUEST::JSON);
	} else {
		HSTREntry entHead, entBody;
		// URL形式でBodyを指定
		for(auto itr=param.cbegin() ; itr!=param.cend() ; itr++)
			entBody[itr.key()] = itr.value();
		entHead.insert("Content-Type", "application/x-www-form-urlencoded");
		entHead.insert("Content-Length", QString("%1").arg(entBody.toURLEncoded(false).length()));

		if(iAuth)
			rep = PostAuthRequest(_amgr, _oauth, cb, url, (int)REQUEST::JSON, entHead, entBody);
		else
			rep = PostRequest(_amgr, cb, url, (int)REQUEST::JSON, entHead, entBody.toURLEncoded(false).toLatin1());
	}
	rep->setProperty(c_prop_uid, uid);
	// 回数制限APIの時はカウンタを減らす
	if(info._bLimited) {
		--_apiLimit.remainHits;
		emit updateAPILimit(_apiLimit);
	}
}
ParamMap TwqNet::DecompParam(const QString &qstr) {
	QStringList sl = qstr.split("?", QString::SkipEmptyParts);
	sl = sl[0].split("&");
	ParamMap pm;
	for(const auto& kp : sl) {
		QStringList ls = kp.split("=");
		pm.insert(ls[0], (ls.size()>1) ? ls[1] : QString(""));
	}
	return pm;
}
void TwqNet::_netResponse(QNetworkReply *rep) {
	REQUEST rt = (REQUEST)(rep->property(c_prop_type).toUInt());
	void* ptr = (void*)rep->property(c_prop_callback).toInt();
	if(rt == REQUEST::REPLY) {
		std::unique_ptr<ReplyCB> p(reinterpret_cast<ReplyCB*>(ptr));
		(*p)(rep);
		return;
	}

	int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if(code/100 != 2) {
		// リクエスト失敗
		QVariant reason = rep->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
		QString msg = QString(tr("error: code=%1 %2")).arg(code).arg(reason.toString());
		qDebug() << msg;
		emit onNetworkError(rep->property(c_prop_uid).toUInt(), reason.toString());
	} else {
		// 成功
		switch(rt) {
			case REQUEST::JSON: {
				std::unique_ptr<RestCB> p(reinterpret_cast<RestCB*>(ptr));
				QByteArray qa(rep->readAll());
				qa.push_back('\0');

				QJsonParseError err;
				QJsonDocument jdoc = QJsonDocument::fromJson(qa, &err);
				if(err.error == QJsonParseError::NoError)
					(*p)(jdoc);
				break; }
			case REQUEST::IMAGE: {
				QPixmap* pm = new QPixmap;
				QImage img;
				std::unique_ptr<ImgCB> p(reinterpret_cast<ImgCB*>(ptr));
				if(pm->loadFromData(rep->readAll()))
					(*p)(pm);
				break;}
			default:
				break;
		}
	}
}
void TwqNet::refresh() const {
	stateChanged(_tokenState);
	updateAPILimit(_apiLimit);
}
TwqNet::TOKEN TwqNet::getTokenState() const {
	return _tokenState;
}
UserID TwqNet::getMyID() const {
	return _myID;
}
void TwqNet::getImage(ImgCB f, const QUrl& url) {
	QUrl turl(url);
	turl.setScheme("http");
	turl.setPort(80);
	GetRequest(_amgr, f, turl, (int)REQUEST::IMAGE);
}
void TwqNet::getJson(RestCB f, const QUrl& url) {
	GetRequest(_amgr, f, url, (int)REQUEST::JSON);
}
const TwqNet::APILimit& TwqNet::getApiLimit() {
	if(getTokenState() != TOKEN::OK)
		_apiLimit.reset();
	return _apiLimit;
}
