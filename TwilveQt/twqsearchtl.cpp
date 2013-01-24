#include <QJsonArray>
#include "twqtlvariant.h"
#include "twqcell.h"
#include "twqtweet.h"
#include "json_kwd.h"
#include "twquser.h"

TwqSearchTL::TwqSearchTL(const QString& keyword, const QString& tkw, QWidget *p):
	TwqTL("", p),
	// JSONを受け取りTLに反映する
	_cbP([this](QJsonDocument& jdoc) {
			auto jobj = jdoc.object();
			auto ar = jobj.find(JSONKWD::SEARCH::Statuses).value().toArray();
			if(ar.size() > 0) {
				// 配列になってるTweetをTweetMgrに登録した後にCellへIDを渡す
				for(auto itr=ar.begin() ; itr!=ar.end() ; itr++) {
					auto twj = (*itr).toObject();
					// ツイート情報登録
					TweetID tid = JtoInt(twj, "id");
					sgTweet.setInfo(tid, twj);

					// ユーザー情報登録
					auto* pp = sgTweet.immGetInfo<TwqTweet::P_User>(tid);
					Q_ASSERT(pp);
					auto twu = twj.find(JSONKWD::TWEET::User);
					sgUser.setInfo(pp->userID(), twu.value().toObject());

					addCell(tid);
				}
				auto meta = jobj.find(JSONKWD::SEARCH::ScMetaData).value().toObject();
				// 次ページのURLが載ってれば続きを読むボタンを有効
				auto itrNext = meta.find(JSONKWD::SEARCH::NextResults);
				if(itrNext != meta.end()) {
					showContinueButton(true);
					_paramNext = TwqNet::DecompParam(itrNext.value().toString());
				} else {
					showContinueButton(false);
					_paramNext.clear();
				}

				// 更新URLを取得
				auto itrRefl = meta.find(JSONKWD::SEARCH::RefreshUrl);
				if(itrRefl != meta.end())
					_paramRefl = TwqNet::DecompParam(itrRefl.value().toString());
				else
					_paramRefl.clear();
			}
			endLoading(QString(tr("query \"%1\" completed. %2 tweet(s) received.")).arg(tlname()).arg(ar.size()));
	})
{
	searchFor(keyword, tkw);
}
void TwqSearchTL::searchFor(const QString &kw, const QString& tkw) {
	setTLName(QString(tr("search for %1 :(%2)")).arg(kw, tkw));
	_keyword = kw;
	_typekwd = tkw;

	ParamMap param;
	param[TWAPIKWD::SC_SEARCH::Q] = kw;
	param[TWAPIKWD::SC_SEARCH::ResultType] = tkw;
	sgNet.restAPI(_cbP, TWAPI::SC_SEARCH, param);
}

void TwqSearchTL::readNextPage() {
	sgNet.restAPI(_cbP, TWAPI::SC_SEARCH, _paramNext);
}

void TwqSearchTL::refreshPage() {
	sgNet.restAPI(_cbP, TWAPI::SC_SEARCH, _paramRefl);
}
