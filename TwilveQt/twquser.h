#ifndef TWQUSER_H
#define TWQUSER_H
#include <list>
#include <unordered_map>
#include <memory>
#include <functional>
#include <QPixmap>
#include "common.h"

// 一定の期間使われなくなったら消す <= あとで実装
// 登録は自動、破棄も自動
// 必要な情報を随時通信で問い合わせ、返答が来た時点でユーザーに返す
// 今持っている情報をフラグで管理
// 全てコールバックの形で受け取る
// Prop用のリクエストをしてからキューに登録
// ステートで管理し、途中で部分的な更新が無いとする
#define sgUser TwqUser::_ref()
//! Userの共有マネージャ
class TwqUser : public QObject, public Singleton<TwqUser> {
	Q_OBJECT
	public:
		//! 情報ブロックの種類
		class P_Name;
		class P_Prop;
		class P_PictureURL;
		class P_Picture;
		class P_Follow;
		class P_Followed;
		typedef CType<P_Name, P_Prop, P_PictureURL, P_Picture, P_Follow, P_Followed> CTInfo;
	private:
		public:
		class User;
		// -------- 取りうるリクエストの定義 --------
		typedef ReqActBase<User> ReqAct;
		typedef std::list<const ReqAct*> RActList;
		class RA_Prop : public ReqAct {
			public:
				RA_Prop();
				void doIt(User& u) const override;
		};
		class RA_Picture : public ReqAct {
			public:
				RA_Picture();
				void doIt(User& u) const override;
		};
		typedef CType<RA_Prop, RA_Picture> CTRAct;

		//! ユーザー情報の中身
		struct User : ResourceBase<User, CTInfo> {
			std::unique_ptr<QPixmap> _pixIcon;			//!< アイコン
			struct {
				bool	bFollowing,
						bFollowed;
				QString	name,
						scrName,
						description,
						profURL;
				int		nTweet,
						nFollow,
						nFollower;
			} _v;

			User(UserID id);
			User(User&& u);
			UserID getID() const;
		};

		const static RA_Prop cs_raProp;
		const static RA_Picture cs_raPic;
		const static ReqAct* cs_RAList[CTRAct::size];

		ResBase<User, UserID, CTRAct, CTInfo>	_base;

	public slots:
		void onNetworkError(uint64_t id, const QString& msg);
	public:
		// -------- ユーザー情報クエリインタフェース群 --------
		//! ユーザー名
		class P_Name : public User {
			public:
				DEFINE_PARTIAL(P_Name, CTInfo)

				const QString& name() const;
				const QString& screenName() const;
		};
		//! プロフィール画像URL
		class P_PictureURL : public P_Name {
			public:
				DEFINE_PARTIAL(P_PictureURL, CTInfo)

				QUrl profileIconURL() const;
		};
		//! プロフィール画像
		class P_Picture : public P_PictureURL {
			public:
				// Propに記載されているプロフィール画像URLが必要
				DEFINE_PARTIAL(P_Picture, CTInfo)
				void receiveInfo(QPixmap* pm);	// CBによる特別扱い

				QPixmap* profileIcon() const;
		};
		//! フォロー関連
		class P_Follow : public P_Name {
			public:
				DEFINE_PARTIAL(P_Follow, CTInfo)
				bool bFollowing() const;
		};
		class P_Followed : public P_Follow {
			public:
				DEFINE_PARTIAL(P_Followed, CTInfo)
				bool bFollowed() const;
		};
		//! User詳細
		class P_Prop : public P_PictureURL {
			public:
				DEFINE_PARTIAL(P_Prop, CTInfo)

				const QString& description() const;
				int nFollow() const;
				int nFollower() const;
				int nTweet() const;
		};

		template <class P>
		const P* immGetInfo(UserID uid) const {
			return _base._immGetInfo<P>(uid);
		}
		template <class P>
		P* immRefInfo(UserID uid) {
			return _base._immRefInfo<P>(uid);
		}

		TwqUser(QObject* parent=nullptr);
		void setInfo(UserID userID, const QJsonObject& jobj) {
			_base.setInfo(userID, jobj);
		}
		template <class... Args, class CB>
		void getInfo(UserID userID, const SPVoid& sp, CB cb) {
			_base.getInfo<Args...>(userID, sp, cb, (const _ReqActBase**)cs_RAList, _countof(cs_RAList));
		}
		template <class P>
		std::function<void (QJsonDocument&)> GetCB(UserID id) {
			return _base.GetCB<P>(_base, id);
		}
		void remInfo(UserID userID) {
			_base.remInfo(userID);
		}
		void clearInfoAll() {
			_base.clearInfoAll();
		}
};

#endif // TWQUSER_H
