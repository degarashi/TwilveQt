#include <QDir>
#include <QApplication>
#include <QJsonArray>
#include "twqtlvariant.h"
#include "twquser.h"
#include "twqtweet.h"
#include "json_kwd.h"
#include "twqcell.h"

namespace {
	const char *c_iconFav = "../image/icon_fav.png",
				*c_iconRT = "../image/icon_rt.png";
}

TwqHomeTL::TwqHomeTL(QWidget *p):
	TwqTL(qApp->translate("TwqHomeTL", "home"), p),
	_cbP([this](QJsonDocument& jdoc) {
		auto jar = jdoc.array();
		for(auto itr=jar.begin() ; itr!=jar.end() ; itr++) {
			// User詳細を持っているならここで初期化しておく
			auto tobj = (*itr).toObject();
			auto uobj = tobj.find(JSONKWD::TWEET::User).value().toObject();
			UserID uid = JtoInt(uobj, JSONKWD::USER::IdStr);
			sgUser.setInfo(uid, uobj);

			// Tweetの登録
			TweetID tid = JtoInt(tobj, JSONKWD::TWEET::IdStr);
			sgTweet.setInfo(tid, tobj);

			addCell(tid);
		}
		endLoading(QString(tr("query \"%1\" completed %2 tweet(s) received.")).arg(tlname()).arg(jar.size()));
		// 次ページは常に存在するものと仮定
		showContinueButton(true);
	})
{
	setTLName(qApp->translate("TwqHomeTL", "home"));
}
void TwqHomeTL::_refreshPage(TWAPI::INDEX idx, const ParamMap &ppm) {
	auto maxid = std::get<1>(idRange());
	ParamMap param(ppm);
	if(maxid != INVALID_TweetID) {
		// ギャップは今のツイートより新しい物を対象
		setGapID(maxid);
		param[TWAPIKWD::TL_HOME::SinceId] = QString("%1").arg(maxid);
	} else {
		setGapID(0);
	}
	// 新しいツイートを取得
	sgNet.restAPI(_cbP, idx, param);
}
void TwqHomeTL::_readNextPage(TWAPI::INDEX idx, const ParamMap &ppm) {
	auto minid = std::get<0>(idRange());
	ParamMap param(ppm);
	if(minid != INVALID_TweetID) {
		// ギャップは今のツイートより古い物を対象
		setGapID(minid);
		param[TWAPIKWD::TL_HOME::MaxId] = QString("%1").arg(minid-1);
	} else {
		setGapID(0);
	}
	sgNet.restAPI(_cbP, idx, param);
}

void TwqHomeTL::refreshPage() {
	ParamMap pm;
	pm.insert(TWAPIKWD::TW_SHOW::IncludeMyRetweet, "true");
	_refreshPage(TWAPI::TL_HOME, pm);
}
void TwqHomeTL::readNextPage() {
	ParamMap pm;
	pm.insert(TWAPIKWD::TW_SHOW::IncludeMyRetweet, "true");
	_readNextPage(TWAPI::TL_HOME, pm);
}
