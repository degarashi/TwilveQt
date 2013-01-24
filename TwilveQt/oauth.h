#ifndef OAUTH_H
#define OAUTH_H

#include <cstdint>
#include <QMap>
#include <QVector>
#include <QString>

class HSTREntry : public QMap<QString, QString> {
	public:
		//! URL形式に変換
		QString toURLEncoded(bool bOAUTH) const;
};
typedef QPair<QString, QString>		HSTRPair;
typedef QVector<HSTRPair>			HSTRMapV;

class QNetworkRequest;
class QUrl;
class OAuth {
	typedef uint8_t SHA1DG[20];

	QString	_token, _token_s,
			_cs_key, _cs_secret,
			_veliID;

	static QString _geneTimeStamp();
	static QString _geneNonce();
	void _calcSHA1(SHA1DG dst, HSTRMapV& sv, const QUrl& url, const QString& cmd) const;

	public:
		// HMAC-SHA1計算
		static void HMAC_SHA1(SHA1DG dst, const char* key, int klen, const char* src, int n);
		static int UrlEncode_OAUTH(char* dst, size_t n_dst, const char* src, int n);
		static int UrlEncode(char* dst, size_t n_dst, const char* src, int n);

		void setCSKey(const QString& key, const QString& key_s);
		void setToken(const QString& tkn, const QString& tkn_s);
		const QString& token() const;
		const QString& secret() const;

		void setVeliID(const QString& id);
		//! HeaderとBodyから計算したOAuth署名を付加する
		void output(QNetworkRequest* req, const QString& cmd, const HSTREntry& addBody) const;
};

#endif // OAUTH_H
