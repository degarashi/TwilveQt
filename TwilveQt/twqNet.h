#ifndef TWQNET_H
#define TWQNET_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QMap>
#include <memory>
#include "common.h"
#include "oauth.h"
#include "api_kwd.h"

QString ConvertHESC(const QString& str, int dir);
QDateTime  ReadDT_Search(const QString& s);
QDateTime ReadDT_Home(const QString& s);

typedef QMap<QString, QString> ParamMap;

class QNetworkReply;
class QJsonDocument;
class QUrl;
class QDir;
class QPixmap;
//! TwitterAPIクエリやHttp通信の管理クラス
// 更新の要求は返答が来るまでに2回以上されたら前の分はキャンセルする
 #define sgNet TwqNet::_ref()
class TwqNet : public QObject, public Singleton<TwqNet> {
	public:
		enum class TOKEN {
			EMPTY,		//!< キーもトークンもない
			KEYS,		//!< キーだけある
			REQUEST,	//!< リクエストトークン要求待ち
			AUTH,		//!< ユーザーの認証待ち(validation ID待ち)
			ACCESS,		//!< アクセストークン要求待ち
			OK			//!< トークンを持っている
		};
		//! APIの呼び出し回数制限
		struct APILimit {
			int remainHits,			//!< 残りREST呼び出し
				resetInSec,			//!< 次のリセット間隔
				hourlyLimit,		//!< 1時間あたりの回数
				// ---- 以下はPhotoに関するリミット ----
				p_remainHits,
				p_resetInSec,
				p_dailyLimit;
			QDateTime   resetTime,
						p_resetTime;

			void reset();
			void readJson(QJsonDocument& jdoc);
		};
		//! 自身に関するTwqUserよりも詳細な情報
		struct MyInfo {

		};

		typedef std::function<void (QPixmap*)>			ImgCB;		//!< イメージ用コールバック
		typedef std::function<void (QJsonDocument&)>	RestCB;		//!< JSON用コールバック
		typedef std::function<void (QNetworkReply*)>	ReplyCB;	//!< その他汎用コールバック

	private:
		Q_OBJECT

		enum class REQUEST {
			JSON,
			IMAGE,
			REPLY,
		};
		typedef uint32_t	TwqID;	//!< レスポンス管理のイベントID

		QNetworkAccessManager	_amgr;
		APILimit				_apiLimit;		//!< API上限管理
		OAuth					_oauth;			//!< OAuth認証クラス
		UserID					_myID;			//!< 自分のTwitterID
		TOKEN					_tokenState;	//!< 認証状態

		void _loadCSKey();
		void _loadToken();
		void _saveToken() const;
		void _openAuthURL();
		void _getAccToken(const QString& valiID);
		QString _loadFromFile(const QDir &path);

	signals:
		void stateChanged(TwqNet::TOKEN state) const;
		void authResult(bool bSuccess, const QString& msg) const;
		void updateAPILimit(const TwqNet::APILimit& lim) const;
		void onNetworkError(uint64_t id, const QString& msg) const;
	private slots:
		void _netResponse(QNetworkReply* rep);
		void _stateChanged(TwqNet::TOKEN state);
	public slots:
		void beginAuthorization();

	public:
		explicit TwqNet(QObject* parent=0);
		TOKEN getTokenState() const;
		void refresh() const;

		//! REST-APIクエリ
		void restAPI(RestCB cb, TWAPI::INDEX index, const ParamMap& param, uint64_t num, uint64_t uid=0);
		void restAPI(RestCB cb, TWAPI::INDEX index, const ParamMap& param, const QString& urlP=QString(), uint64_t uid=0);
		//! クエリ文字列を分解してParamMapの形式にする
		static ParamMap DecompParam(const QString& qstr);
		//! Httpによる画像取得
		void getImage(ImgCB cb, const QUrl& url);
		//! HttpによるJson取得
		void getJson(RestCB cb, const QUrl& url);

		const APILimit& getApiLimit();
		//! 自身のユーザーID
		UserID getMyID() const;
		//! 一意のリクエストIDを受け取る
		ReqID newRequestID();
};

#endif // TWQNET_H
