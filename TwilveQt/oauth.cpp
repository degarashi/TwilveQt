#include "oauth.h"
#include "sha1.h"
#include <stdexcept>
#include <QNetworkRequest>
#include <QTextStream>
#include <QUrlQuery>
#include <QTime>
#include <chrono>

QString HSTREntry::toURLEncoded(bool bOAUTH) const {
	QString ret;
	bool bFirst = true;
	// 登録順に書きだす
	for(auto itr=this->cbegin() ; itr!=this->cend() ; itr++) {
		if(bFirst)
			bFirst = false;
		else
			ret += "&";

		auto pF = (bOAUTH) ? OAuth::UrlEncode_OAUTH : OAuth::UrlEncode;
		// url(Entry)=url(Value)
		char tb[2048];
		char *tb_ptr = tb,
			  *tb_end = tb+sizeof(tb);
		QByteArray ut = itr.key().toUtf8();
		tb_ptr += pF(tb_ptr, tb_end-tb_ptr, ut.data(), ut.size());
		*tb_ptr++ = '=';
		ut = itr.value().toUtf8();
		tb_ptr += pF(tb_ptr, tb_end-tb_ptr, ut.data(), ut.size());
		*tb_ptr = '\0';

		ret += tb;
	}
	return ret;
}
void OAuth::HMAC_SHA1(SHA1DG dst, const char* key, int klen, const char* src, int n) {
	char tmp_key[128];
	if(klen > 64) {
		SHA1DG keydg;
		SHA1Context sha;
		SHA1Reset(&sha);
		SHA1Input(&sha, (const uint8_t*)key, klen);
		SHA1Result(&sha, keydg);
		HMAC_SHA1(dst, (const char*)keydg, 20, src, n);
		return;
	}

	memcpy(tmp_key, key, klen);
	for(int i=klen ; i<64+1 ; i++)
		tmp_key[i] = 0x00;

	char tmp_0[64], tmp_1[64];
	for(int i=0 ; i<64 ; i++) {
		tmp_0[i] = tmp_key[i] ^ 0x5c;
		tmp_1[i] = tmp_key[i] ^ 0x36;
	}

	SHA1Context sha;
	SHA1Reset(&sha);
	SHA1Input(&sha, (const uint8_t*)tmp_1, 64);
	SHA1Input(&sha, (const uint8_t*)src, n);
	SHA1Result(&sha, dst);

	SHA1Reset(&sha);
	SHA1Input(&sha, (const uint8_t*)tmp_0, 64);
	SHA1Input(&sha, dst, 20);
	SHA1Result(&sha, dst);
}

namespace {
	bool IsURLChar_OAUTH(char c) {
		return (c>='A' && c<='Z') || (c>='0' && c<='9') || (c>='a' && c<='z') ||
			c=='.' || c=='-' || c=='_' || c=='~';
	}
	char Get16Char_OAUTH(int num) {
		if(num >= 10)
			return 'A'+num-10;
		return '0'+num;
	}

	bool IsURLChar(char c) {
		return (c>='A' && c<='Z') || (c>='0' && c<='9') || (c>='a' && c<='z') ||
			c=='\'' || c=='.' || c=='-' || c=='*' || c==')' || c=='(' || c=='_';
	}
	char Get16Char(int num) {
		if(num >= 10)
			return 'a'+num-10;
		return '0'+num;
	}
}
int OAuth::UrlEncode_OAUTH(char* dst, size_t n_dst, const char* src, int n) {
	int wcur = 0,
		rcur = 0;
	for(;;) {
		unsigned char c = src[rcur++];
		if(--n < 0)
			break;
		else if(IsURLChar_OAUTH(c))
			dst[wcur++] = c;
		else {
			dst[wcur++] = '%';
			dst[wcur++] = Get16Char_OAUTH(c>>4);
			dst[wcur++] = Get16Char_OAUTH(c&0x0f);
		}
	}
	dst[wcur] = '\0';
	if(wcur >= (int)n_dst)
		throw std::length_error("url_encode_OAUTH(): buffer overflow");
	return wcur;
}
int OAuth::UrlEncode(char* dst, size_t n_dst, const char* src, int n) {
	int wcur = 0,
		rcur = 0;
	for(;;) {
		unsigned char c = src[rcur++];
		if(--n < 0)
			break;
		else if(IsURLChar(c))
			dst[wcur++] = c;
		else if(c == ' ')
			dst[wcur++] = '+';
		else {
			dst[wcur++] = '%';
			dst[wcur++] = Get16Char(c>>4);
			dst[wcur++] = Get16Char(c&0x0f);
		}
	}
	dst[wcur] = '\0';
	if(wcur >= (int)n_dst) {
		//throw std::length_error("url_encode(): buffer overflow");
	}
	return wcur;
}

namespace {
	// 文字列定数など
	namespace OAUTH {
		namespace ENT {
			const static QString  TIMESTAMP = "oauth_timestamp",
						NONCE = "oauth_nonce",
						CALLBACK = "oauth_callback",
						SIGNATURE = "oauth_signature",
						VERSION = "oauth_version",
						SIG_METHOD = "oauth_signature_method",
						CS_KEY = "oauth_consumer_key",
						VERIFIER = "oauth_verifier",
						TOKEN = "oauth_token",
						TOKENS = "oauth_token_secret";
		}
		const static QString HMAC_SHA1 = "HMAC-SHA1",
					_1_0 = "1.0",
					HTTP1_1 = "HTTP/1.1",
					OAUTH = "OAuth",
					AUTHZ = "Authorization";
					//*HTTPHEAD = "http://";
	}
}

void OAuth::setCSKey(const QString &key, const QString &key_s) {
	Q_ASSERT(!key.isEmpty() && !key_s.isEmpty());
	_cs_key = key;
	_cs_secret = key_s;
}
void OAuth::setToken(const QString &tkn, const QString &tkn_s) {
	_token = tkn;
	_token_s = tkn_s;
}

const QString& OAuth::token() const { return _token; }
const QString& OAuth::secret() const { return _token_s; }
void OAuth::setVeliID(const QString& id) {
	_veliID = id;
}
QString OAuth::_geneTimeStamp() {
	return QString("%1").arg(QDateTime::currentMSecsSinceEpoch()/1000);
}
QString OAuth::_geneNonce() {
	auto t = std::chrono::system_clock::now();
	auto t2 = std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch());
	return QString("%1").arg(t2.count());
}

namespace {
	QByteArray ToBA(const QString& str) {
		return QByteArray(str.toLocal8Bit(), str.size());
	}
}

void OAuth::_calcSHA1(SHA1DG dst, HSTRMapV& sv, const QUrl& url, const QString& cmd) const {
	// アルファベット順にソート
	std::sort(sv.begin(), sv.end(), [](const HSTRPair& s0, const HSTRPair& s1) -> bool{
		return s0.first < s1.first;
	});

	// ハッシュキー生成
	char tb_key[512];
	strcpy(tb_key, _cs_secret.toLocal8Bit());
	strcat(tb_key, "&");
	strcat(tb_key, _token_s.toLocal8Bit());

	// ハッシュ文字列生成
	// :コマンドとURL
	const int BUFF_SIZE = 16384;
	char tb_bstr[BUFF_SIZE];
	char *tb_ptr = tb_bstr,
			*tb_end = tb_bstr+BUFF_SIZE;
	QByteArray cmd8 = cmd.toUtf8();
	tb_ptr += UrlEncode_OAUTH(tb_ptr, tb_end-tb_ptr, cmd8.data(), cmd8.size());
	*tb_ptr++ = '&';
	QByteArray url8 = url.toString(QUrl::RemoveQuery).toUtf8();
	tb_ptr += UrlEncode_OAUTH(tb_ptr, tb_end-tb_ptr, url8.data(), url8.size());
	*tb_ptr++ = '&';

	// :パラメタ
	char tb_tmp[4096];
	QByteArray tmp;
	QTextStream ss(&tmp);
	int nA = sv.size();
	for(int i=0 ; i<nA ; i++) {
		if(i!=0)
			ss << '&';
		HSTRPair& sp = sv[i];
		ss << sp.first.toUtf8() << '=';
		QByteArray ba = sp.second.toUtf8();
		size_t n = UrlEncode_OAUTH(tb_tmp, sizeof(tb_tmp), ba.data(), ba.size());
		Q_ASSERT(n < sizeof(tb_tmp)-1);
		tb_tmp[n] = '\0';
		ss << tb_tmp;
	}
	ss.flush();
	tb_ptr += UrlEncode_OAUTH(tb_ptr, tb_end-tb_ptr, tmp.data(), tmp.size());
	Q_ASSERT(tb_ptr <= tb_end);
	tb_ptr[0] = '\0';
	tb_key[strlen(tb_key)] = '\0';

	HMAC_SHA1(dst, tb_key, strlen(tb_key), tb_bstr, tb_ptr-tb_bstr);
}
void OAuth::output(QNetworkRequest *req, const QString& cmd, const HSTREntry& addBody) const {
	Q_ASSERT(!_cs_key.isEmpty() && !_cs_secret.isEmpty());
	// OAuthヘッダを生成
	HSTREntry ent;
	ent[OAUTH::ENT::TIMESTAMP] = _geneTimeStamp();
	ent[OAUTH::ENT::NONCE] = _geneNonce();
	ent[OAUTH::ENT::VERSION] = OAUTH::_1_0;
	ent[OAUTH::ENT::SIG_METHOD] = OAUTH::HMAC_SHA1;
	ent[OAUTH::ENT::CS_KEY] = _cs_key;
	if(!_token.isEmpty())
		ent[OAUTH::ENT::TOKEN] = _token;
	else
		ent[OAUTH::ENT::CALLBACK] = "oob";
	if(!_veliID.isEmpty())
		ent[OAUTH::ENT::VERIFIER] = _veliID;
	// ソートの為に配列へ書き出し
	HSTRMapV sv;
	for(auto itr=ent.cbegin() ; itr!=ent.cend() ; itr++)
		sv.push_back(qMakePair(itr.key(), itr.value()));

	// add query items (for GET)
	if(cmd == "GET") {
		QUrlQuery q(req->url());
		auto qil = q.queryItems();
		for(auto qi : qil)
			sv.push_back(HSTRPair(qi.first, qi.second));
	} else if(cmd == "POST") {
		// 本体ノードがあればそれも加味
		// add body items (for POST)
		for(auto itr=addBody.cbegin() ; itr!=addBody.cend() ; itr++)
			sv.push_back(HSTRPair(itr.key(), itr.value()));
	} else {
		Q_ASSERT(false);
	}

	// 署名を計算
	SHA1DG dg;
	_calcSHA1(dg, sv, req->url(), cmd);

	// :base64変換
	QByteArray dg64 = QByteArray((const char*)dg, 20).toBase64();
	// :url(OAuth)変換
	char tc[64];
	UrlEncode_OAUTH(tc, sizeof(tc), dg64.data(), dg64.size());
	// 署名を付加
	ent.insert(OAUTH::ENT::SIGNATURE, tc);

	// Requestへ追加
	QString ss(OAUTH::OAUTH);
	QChar ch(' ');
	for(auto itr=ent.cbegin() ; itr!=ent.cend() ; itr++) {
		ss += QString("%1%2=\"%3\"").arg(ch).arg(itr.key()).arg(itr.value());
		ch = ',';
	}
	req->setRawHeader(ToBA(OAUTH::AUTHZ), ToBA(ss));
}
