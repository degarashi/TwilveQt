#ifndef TWQTWEET_H
#define TWQTWEET_H

#include <QObject>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <unordered_map>
#include <list>
#include "common.h"

#define sgTweet TwqTweet::_ref()
//! Tweetの共有マネージャ
class TwqTweet : public QObject, public Singleton<TwqTweet> {
	Q_OBJECT
	public:
		class P_Base;
		class P_User;
		class P_Home;
		class P_RT;
		class P_Full;
		typedef CType<P_Base, P_User, P_Home, P_RT, P_Full> CTInfo;

	private:
		struct Tweet : ResourceBase<Tweet, CTInfo> {
			struct {
				QDateTime	created;
				TweetID		toTweetID,	//!< 返信先のID
							reTweetID,	//!< これ自体が非RTの時に有効: RTのID
							srcTweetID;	//!< これ自体がRTの時に有効: 元ID
				QString		text,
							source;
				UserID		userID,
							toUserID;
				bool		bRetweeted,
							bFavorited;
				int			rtCount;
			} _v;
			Tweet(TweetID id);
			Tweet(Tweet&& t);
			TweetID getID() const;
		};

		// -------- 取りうるリクエストの定義 --------
		// 今の所Fullだけ定義
		class RA_Full : public ReqActBase<Tweet> {
			public:
				RA_Full();
				void doIt(Tweet& t) const override;
		};
		typedef CType<RA_Full> CTRAct;

		typedef ReqActBase<Tweet>			ReqAct;
		typedef std::list<const ReqAct*>	RActList;
		const static RA_Full cs_raFull;
		const static ReqAct* cs_RAList[CTRAct::size];

		ResBase<Tweet, TweetID, CTRAct, CTInfo>	_base;

	public:
		// -------- ユーザー情報クエリインタフェース群 --------
		class P_Base : public Tweet {
			public:
				DEFINE_PARTIAL(P_Base, CTInfo)

				QDateTime createdAt() const;
				QString text() const;
				QString source() const;
		};
		class P_User : public P_Base {
			public:
				DEFINE_PARTIAL(P_User, CTInfo)

				UserID userID() const;
				UserID toUserID() const;
		};
		// from HomeTL
		class P_Home : public P_User {
			public:
				DEFINE_PARTIAL(P_Home, CTInfo)

				TweetID toTweetID() const;
				bool isFavorited() const;
				//! これがtrueならRT元ツイートを意味する
				bool isReTweeted() const;
				int rtCount() const;
		};
		class P_RT : public P_Home {
			public:
				DEFINE_PARTIAL(P_RT, CTInfo)

				//! "自分の" ReTweetIDを取得
				/*! \return 無ければINVALID_TweetIDが返る */
				TweetID reTweetID() const;
				//! RT元のIDを取得
				TweetID srcID() const;
				bool isRT() const;
		};
		// has TwqUser(Full)
		class P_Full : public P_Home {
			public:
				DEFINE_PARTIAL(P_Full, CTInfo)
		};

	public:
		TwqTweet(QObject* parent=nullptr);
		template <class P>
		const P* immGetInfo(TweetID twID) const {
			return _base._immGetInfo<P>(twID);
		}
		template <class P>
		P* immRefInfo(TweetID twID) {
			return _base._immRefInfo<P>(twID);
		}
		template <class P>
		std::function<void (QJsonDocument&)> GetCB(TweetID id) {
			return _base.GetCB<P>(_base, id);
		}
		void setInfo(TweetID twID, const QJsonObject& jobj) {
			_base.setInfo(twID, jobj);
		}
		template <class... Args, class CB>
		void getInfo(TweetID twID, const SPVoid& sp, CB cb) {
			_base.getInfo<Args...>(twID, sp, cb, (const _ReqActBase**)cs_RAList, _countof(cs_RAList));
		}
		void remInfo(TweetID twID) {
			_base.remInfo(twID);
		}
		void clearInfoAll() {
			_base.clearInfoAll();
		}
};

#endif // TWQTWEET_H
