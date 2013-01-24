#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <climits>
#include <QObject>
#include <QException>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <unordered_map>
#include <QDebug>
#include "type.h"

//! ユーザーの操作
enum class USEROP {
	USER_REPLY,				//!< ユーザーにリプライ (UserID)
	USER_FOLLOW,			//!< 誰かをフォロー (UserID)
	USER_UNFOLLOW,			//!< 誰かのフォローを解除 (UserID)
	USER_BLOCK,				//!< ユーザーをブロック (UserID)
	USER_UNBLOCK,			//!< ユーザーのブロックを解除 (UserID)
	TWEET_REPLY,			//!< (TweetID)
	TWEET_OFCRT,			//!< 公式リツイート (TweetID)
	TWEET_NOFCRT,			//!< 非公式リツイート (TweetID)
	TWEET_CANCEL_OFCRT,		//!< (TweetID)
	TWEET_QT,				//!< (TweetID)
	TWEET_FAVORITE,			//!< ツイートをふぁぼ (TweetID)
	TWEET_UNFAVORITE,		//!< ツイートのふぁぼを取り消し (TweetID)
	TWEET_REMOVE,			//!< (TweetID)
	TWEET					//!< ツイートを送信 (Message)
};

//! TwqMainから発せられるTwqイベントID
enum class TWQEVENT {
	USER_REPLY,				//!< ユーザーにリプライ
	USER_FOLLOW,			//!< 誰かをフォローした
	USER_UNFOLLOW,			//!< 誰かのフォローを解除した
	USER_BLOCK,				//!< ユーザーをブロック
	USER_UNBLOCK,			//!< ユーザーのブロックを解除
	TWEET_MODIFIED,			//!< (ユーザーのアクションにより)ツイートの状態が変更された
	TWEET_ADDED,			//!< ツイートが新たに加わった
	TWEET_REMOVED			//!< ツイートが削除された
};
/*! UserJ = (QMap: id=UserID, json=UserJson)
	TweetJ = (QMap: id=TweetID, json=TweetJson) */
enum class TWQEVENTTYPE {
	USER,		//!< param = UserJ
	TWEET		//!< param = TweetJ
};
inline TWQEVENTTYPE DetectEventType(TWQEVENT e) {
	int ie = (int)e;
	if(ie >= (int)TWQEVENT::TWEET_MODIFIED && ie <= (int)TWQEVENT::TWEET_REMOVED)
		return TWQEVENTTYPE::TWEET;
	return TWQEVENTTYPE::USER;
}

inline uint64_t JtoInt(const QJsonObject& jo, const char* ent) {
	return jo.find(ent).value().toVariant().toULongLong();
}
typedef uint64_t	UserID;			//!< ユーザー固有の値
typedef uint64_t	TweetID;		//!< ツイート固有の値
typedef uint32_t	ReqID;			//!< ネットワークリクエストID
const static TweetID INVALID_TweetID = std::numeric_limits<TweetID>::max();
const static UserID INVALID_UserID = std::numeric_limits<UserID>::max();
const static ReqID INVALID_ReqID = std::numeric_limits<ReqID>::max();
//! 必要な情報フラグ
typedef uint32_t NeedFlag;
typedef NeedFlag State;
//! コールバック生存確認の為のスマートポインタ
typedef std::shared_ptr<void> SPVoid;
typedef std::weak_ptr<void> SPVoidW;

//! リソースクエリのコールバック
template <class US>
struct ReqData {
	typedef std::function<void (const US&)> ReqCB;
	NeedFlag		need;		//!< 要求達成の条件
	ReqCB			callback;	//!< コールバック関数
	SPVoidW			wptr;		//!< コールバック先の生存確認

	ReqData(ReqData&& u) {
		swap(u);
	}
	ReqData(NeedFlag nflag, const SPVoid& sp, ReqCB cb):
		need(nflag), callback(cb), wptr(sp)
	{}
	void swap(ReqData& u) noexcept {
		need = u.need;
		callback.swap(u.callback);
		wptr.swap(u.wptr);
	}
};
/*! どんな情報が必要か : どのように問い合わせするか
	それぞれの情報に対してRequest手段が設けられている
	\param ptype 対象の型
	\param ctinfo 型リスト */
#define DEFINE_PARTIAL(ptype, ctinfo) \
	const static NeedFlag Flag = 1 << ctinfo::Find<ptype>::result; \
	void receiveInfo(const QJsonObject& jobj);

//! 必要な情報を持っているかのチェックと変数の採取
void DUMMY_receiveInfo(QJsonObject& jobj);

//! CTInfoの型を巡回
template <class  CT, int N=CT::size-1>
struct CTItr {
	typedef typename CT::First F;
	typedef typename CT::Other OTHER;
	//! 初回のフラグ回収 & Info集め
	static void proc(void* dst, const QJsonObject& jobj) {
		auto* pDst = reinterpret_cast<F*>(dst);
		pDst->receiveInfo(jobj);
		CTItr<OTHER, N-1>::proc(dst, jobj);
	}
};
template <class CT>
struct CTItr<CT,-1> {
	static void proc(void* dst, const QJsonObject& jobj) {}
};

class PlanHist;
class _ReqActBase {
	int			_cost;
	NeedFlag	_need, _comp;

	public:
		_ReqActBase(NeedFlag need, NeedFlag comp, int cost);
		bool canDo(State state) const;
		PlanHist simulate(const PlanHist& hist) const;
		NeedFlag getComp() const;

		//! to create virtual function table
		virtual void  dummy() {}
};

template <class T>
class ReqActBase : public _ReqActBase {
	public:
		ReqActBase(NeedFlag need, NeedFlag comp, int cost): _ReqActBase(need,comp,cost) {}
		virtual void doIt(T& t) const = 0;
};
typedef std::list<const _ReqActBase*> RActList;

struct PlanHist {
	State		state;
	RActList	plan;
	int			cost;

	PlanHist();
	PlanHist(const PlanHist& hist);
	PlanHist(PlanHist&& hist);
	PlanHist& operator = (const PlanHist& hist);
	void swap(PlanHist& hist) noexcept;
	void invalidate();
	bool valid() const;
};
//! 目的の情報を得るまでの動作プラン
struct Plan {
	State		final;		//!< 目標状態
	RActList	rcand;		//!< これから取りうるアクション候補
	PlanHist	current,	//!< 現在までの経過
				best;		//!< 最終結果 (ベストな経路)

	bool isFinished() const;
	//! 再帰呼び出しによる経路探索
	void _planning();
	//! リクエストアクション経路探索
	PlanHist planning(NeedFlag cur, NeedFlag fnl, const _ReqActBase** acts, size_t nAct);
};

inline uint32_t AllOR() {
	return 0;
}
//! 与えられた引数を全てOrする関数
template <class T, class... Args>
inline uint32_t AllOR(T first, Args... args) {
	return first | AllOR(args...);
}
//! T=ユーザーリソース型
template <class T, class CTInfo>
struct ResourceBase {
	typedef ReqActBase<T>				ReqAct;
	typedef std::list<const ReqAct*>	RActList;
	typedef ReqData<T>					UserReq;	//!< ユーザー情報クエリのコールバック
	typedef std::list<UserReq>			ReqQ;

	uint64_t		id;							//!< 自身のID
	NeedFlag		haveF;						//!< 既に持っている情報フラグ
	RActList		reqAct;						//!< 次のリクエスト
	ReqQ			reqQ;						//!< 要求待ちコールバック
	bool			bReq;						//!< 現在リクエスト中かどうか

	ResourceBase(uint64_t id): id(id), haveF(0), bReq(false) {}
	//! 次のリクエストアクションを実行
	void doRequestAction() {
		if(!reqAct.empty()) {
			// 次のリクエストをキューから出して実行
			auto* act = reqAct.front();
			act->doIt(reinterpret_cast<T&>(*this));
			reqAct.pop_front();
		}
	}
	//! リプライの受信処理
	void receiveReply(NeedFlag compFlag) {
		// 情報フラグの更新
		haveF |= compFlag;
		// このリプライを貰うことで条件を達成する要求に対してコールバックを呼ぶ
		for(auto itr=reqQ.begin() ; itr!=reqQ.end() ; itr++) {
			auto& p = *itr;
			// 条件を満たすか？
			if((p.need & haveF) == p.need) {
				if(auto wp = p.wptr.lock()) {
					// コールバックを読んでキューから除く
					p.callback(reinterpret_cast<T&>(*this));
					itr = reqQ.erase(itr);
				} else {
					Q_ASSERT(false);
				}
			}
		}
		// 次のリクエストを実行
		doRequestAction();
	}
	//! 入力される情報が型の要求する条件に合致しているか
	void checkFlag(const QJsonObject& jobj) {
		// CTInfoに登録されているクラスを順に調べる
		haveF = CTItr<CTInfo>::proc(this, jobj);
	}
};

// 汎用シングルトンクラス
template <typename T>
class Singleton {
	private:
		static T* ms_singleton;
	public:
		Singleton() {
			Q_ASSERT(!ms_singleton || !"initializing error - already initialized");
			int offset = (int)(T*)1 - (int)(Singleton<T>*)(T*)1;
			ms_singleton = (T*)((int)this + offset);
		}
		virtual ~Singleton() {
			Q_ASSERT(ms_singleton || !"destructor error");
			ms_singleton = 0;
		}
		static T& _ref() {
			Q_ASSERT(ms_singleton || !"reference error");
			return *ms_singleton;
		}
};
/*! \param TResource リソース型
	\param TResID リソースID
	\param TRAct リクエストアクションタイプリスト
	\param TRInfo 情報ブロックタイプリスト */
template <class TResource, class TResID, class TRAct, class TRInfo>
class ResBase {
	typedef ReqData<TResource>		ResReq;
	typedef std::list<ResReq>		ReqList;
	typedef ReqActBase<TResource>	ReqAct;

	typedef std::unordered_map<TResID, TResource>	ResMap;
	ResMap	_resMap;
	public:
		//! 他のAPI呼び出しでついでに得られた情報など格納, フラグ更新
		/*! もし既に持っている情報と重複していれば何もしない
			与えられたJSONは常に最新の物として扱う */
		void setInfo(TResID id, const QJsonObject& jobj) {
			auto itr = _resMap.find(id);
			if(itr == _resMap.end()) {
				_resMap.insert(std::make_pair(id, TResource(id)));
				itr = _resMap.find(id);
				itr->second.id = id;
			}
			// PartialInfoを順に調べてCheckInfo() -> receiveInfo()を呼び出す
			CTItr<TRInfo>::proc(&itr->second, jobj);
		}

		void removeInfo(TResID id) {
			auto itr = _resMap.find(id);
			if(itr != _resMap.end())
				_resMap.erase(itr);
		}

		//! 任意の粒度でユーザー情報アクセス
		/*! もしまだユーザーエントリが作られていなければ新しく作る
			\return リクエスト管理のID */
		// テンプレート引数は後で数値指定に改良
		template <class... Args, class CB>
		void getInfo(TResID id, const SPVoid& sp, CB cb, const _ReqActBase** ractSrc, size_t nAct) {
			auto itr = _resMap.find(id);
			if(itr == _resMap.end()) {
				_resMap.insert(std::make_pair(id, TResource(id)));
				itr = _resMap.find(id);
				itr->second.id = id;
			}
			auto& u = itr->second;

			// 型から必要な情報を探る
			NeedFlag needF = AllOR((Args::Flag)...);
			// 必要な情報が既に揃ってるならすぐコールバックを呼ぶ
			if((needF & u.haveF) == needF) {
				// 任意の数の引数をCallback関数で受け取る機構 (明示的なテンプレート指定が必要)
				cb((const Args&)u...);
			} else {
				Plan planner;
				auto plan = planner.planning(u.haveF, needF, ractSrc, nAct);
				// 既に前のアクションリストを持っていたら上書きする
				// （本当は）リストの先頭のみが実行中であるのでそれ以降をすりあわせれば済む
				// リクエストアクションの先頭から1つずつ実行
				Q_ASSERT(!plan.plan.empty());
				u.reqAct.clear();
				for(auto* p : plan.plan)
					u.reqAct.push_back(reinterpret_cast<const ReqAct*>(p));
				auto ncb = [this, cb](const TResource& u2) {
					cb((const Args&)u2...);
				};
				u.reqQ.push_back(ResReq(needF, sp, ncb));
				u.doRequestAction();
			}
		}

		//! ユーザーエントリを持っているか(即時クエリ)
		/*! \return エントリがあればそのポインタ、なければnull */
		template <class P>
		const P* _immGetInfo(TResID id) const {
			auto itr = _resMap.find(id);
			if(itr != _resMap.end()) {
				// 要求するフラグを持っているか
				const TResource* p = &itr->second;
				if((p->haveF & P::Flag) == P::Flag)
					return reinterpret_cast<const P*>(p);
			}
			return nullptr;
		}

		//! エントリを直に取得 (内部処理用)
		template <class P>
		P* _immRefInfo(TResID id) {
			auto itr = _resMap.find(id);
			if(itr != _resMap.end())
				return reinterpret_cast<P*>(&itr->second);
			return nullptr;
		}
		template <class P>
		static std::function<void (QJsonDocument&)> GetCB(ResBase& src, TResID id) {
			return [&src, id](QJsonDocument& jd) {
				// リプライが届くまでにユーザー情報が破棄されるケースを考慮
				auto* ent = src._immRefInfo<P>(id);
				// エントリが現存しているか？
				if(ent) {
					// 配列であれば0番要素を取り出す
					auto jobj = (jd.isArray()) ? jd.array().at(0).toObject() : jd.object();
					// 連想配列: 単体オブジェクトとみなす
//					ent->receiveInfo(jobj);
					src.setInfo(id, jobj);
					// 完了フラグをつける & 次のリクエストを送ったりコールバック関数を呼んだりの内部処理
					ent->receiveReply(P::Flag);
				}
			};
		}
		void remInfo(TResID id) {
			auto itr = _resMap.find(id);
			if(itr != _resMap.end())
				_resMap.erase(itr);
		}
		void clearInfoAll() {
			_resMap.clear();
		}
};


template <typename T>
T* Singleton<T>::ms_singleton = nullptr;

#define _countof(a) sizeof(a)/sizeof((a)[0])
// --- このプログラム全体で使う例外クラス ---
//! 例外既定
class ExceptionBase : public QException {
	QString _whatQs;
	public:
		ExceptionBase(const QString& type, const QString& what): _whatQs(type + ": " + what) {}
		virtual ~ExceptionBase() noexcept {}
		const QString whatQs() const { return _whatQs; }
};
//! 不正ファイル例外
/*! システムファイルの中身が不正な時の例外 */
class InvalidFileException : public ExceptionBase {
	public:
		InvalidFileException(const QString& what): ExceptionBase("InvalidFileException", what) {}
};
//! 実行時エラー例外
/*! 主に外部入力ファイルが読めなかったり等プログラムの外部に起因するエラー */
class RuntimeException : public ExceptionBase {
	public:
		RuntimeException(const QString& what): ExceptionBase("RuntimeException", what) {}
};
//! ロジック例外
/*! プログラム内部の処理が矛盾している時のエラー */
class LogicException : public ExceptionBase {
	public:
		LogicException(const QString& what): ExceptionBase("LogicException", what) {}
};
//! 引数例外
/*! 関数やクラスに与えられた引数が不正な時のエラー */
class InvalidArgsException : public ExceptionBase {
	public:
		InvalidArgsException(const QString& what): ExceptionBase("InvalidArgsException", what) {}
};

#endif // COMMON_H
