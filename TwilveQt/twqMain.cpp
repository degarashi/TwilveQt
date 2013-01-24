#include <QLayout>
#include <QMessageBox>
#include <QDebug>
#include "twqMain.h"
#include "twqNet.h"
#include "ui_mainwindow.h"
#include "dlg_tweet.h"
#include "dlg_addsearch.h"
#include "twqtl.h"
#include "twqtlvariant.h"
#include "json_kwd.h"
#include "twquser.h"
#include "twqtweet.h"
#include <QFile>
const char* TwqMain::cs_menu[] = {
	QT_TR_NOOP("Application(&F)"),
	QT_TR_NOOP("Column(&C)"),
	QT_TR_NOOP("Action(&A)")
};
const TwqMain::ActionDef TwqMain::cs_action[] = {
	//! アプリケーションに関するアクション
	{MENU::APPLICATION, QT_TR_NOOP("Authorization(&A)"), "Ctrl+A"},
	{MENU::APPLICATION, QT_TR_NOOP("API Limit(&L)"), "Ctrl+L"},
	{MENU::APPLICATION, QT_TR_NOOP("Quit(&Q)"), "Ctrl+Q"},
	//! TLの操作に関するアクション
	{MENU::COLUMN, QT_TR_NOOP("Open search(&S)"), "Ctrl+S"},
	{MENU::COLUMN, QT_TR_NOOP("Open home(&H)"), "Ctrl+H"},
	{MENU::COLUMN, QT_TR_NOOP("Open mentions(&M)"), "Ctrl+M"},
	//! 自発的なアクション
	{MENU::ACTION, QT_TR_NOOP("Tweet(&T)"), "Ctrl+T"}
};

TwqMain::TwqMain(QWidget *parent): QMainWindow(parent), _ui(new Ui::MainWindow), _lbAPILimit(new QLabel()), _spVoid(new int) {
	_ui->setupUi(this);

	// メニューの初期化
	for(int i=0 ; i<static_cast<int>(_countof(cs_menu)) ; i++) {
		_menu[i] = new QMenu(tr(cs_menu[i]));
		menuBar()->addMenu(_menu[i]);
	}
	// メニューに対応するアクションの初期化
	for(int i=0 ; i<static_cast<int>(_countof(cs_action)) ; i++) {
		auto& ac = cs_action[i];
		auto act = new QAction(tr(ac.name), nullptr);
		act->setShortcut(QKeySequence(ac.shortcut));
		_menu[(int)ac.menuID]->addAction(act);
		_action[i] = act;
	}
	statusBar()->addPermanentWidget(_lbAPILimit.get());

	// アクションをスロットに割り当てる
	connect(_action[(int)ACTION::APP_AUTH], SIGNAL(triggered()), &sgNet, SLOT(beginAuthorization()));
	connect(_action[(int)ACTION::APP_QUIT], SIGNAL(triggered()), this, SLOT(quit()));
	connect(_action[(int)ACTION::TWEET], SIGNAL(triggered()), this, SLOT(_openTweetWindow()));
	connect(_action[(int)ACTION::COLUMN_SEARCH], SIGNAL(triggered()), this, SLOT(addTLSearch()));
	connect(_action[(int)ACTION::COLUMN_HOME], SIGNAL(triggered()), this, SLOT(addTLHome()));
	connect(_action[(int)ACTION::COLUMN_MENTIONS], SIGNAL(triggered()), this, SLOT(addTLMentions()));
	//TODO: API v1.1にまだ未対応なので封印
//	connect(_action[(int)ACTION::APP_LIMIT], SIGNAL(triggered()), this, SLOT(showAPILimit()));
	_action[(int)ACTION::APP_LIMIT]->setEnabled(false);

	connect(&sgNet, SIGNAL(stateChanged(TwqNet::TOKEN)), this, SLOT(stateChanged(TwqNet::TOKEN)));
	connect(&sgNet, SIGNAL(authResult(bool,QString)), this, SLOT(authResult(bool,QString)));
	connect(&sgNet, SIGNAL(updateAPILimit(TwqNet::APILimit)), this, SLOT(updateAPILimit(TwqNet::APILimit)));
	connect(&sgNet, SIGNAL(onNetworkError(uint64_t,QString)), this, SLOT(onNetworkError(uint64_t,QString)));

	stateChanged(sgNet.getTokenState());
	centralWidget()->setLayout(new QHBoxLayout);
	setMaximumWidth(320);
}
void TwqMain::onNetworkError(uint64_t ucode, const QString& msg) {
	showStatus(tr("network error: %1").arg(msg));
}
void TwqMain::_openTweetWindow() {
	_openTweetWindow(QString(), QString());
}
void TwqMain::_openTweetWindow(const QString& pre, const QString& post) {
	DlgTweet dlg;
	QString str = dlg.exec(pre, post);
	if(!str.isNull()) {
		// つぶやき送信
		userOp(USEROP::TWEET, str);
	}
}

void TwqMain::showStatus(const QString& stat) {
	// 表示時間は一律10秒とする
	statusBar()->showMessage(stat, 10000);
}
namespace {
	void Tmp(TWQEVENT e, TweetID id, const QString& stat) {
		sgMain.showStatus(stat);
		emit sgMain.twqEvent(e, id);
	}
	void Tmp(TWQEVENT e, const QJsonObject& obj, const QString& stat) {
		TweetID id = JtoInt(obj, JSONKWD::TWEET::IdStr);
		sgTweet.setInfo(id, obj);
		Tmp(e, id, stat);
	}
}
void TwqMain::userOp(USEROP op, const QVariant& value) {
	ParamMap pm;
	switch(op) {
		case USEROP::TWEET:
			pm.insert(TWAPIKWD::TW_UPDATE::Status, value.toString());
			sgNet.restAPI([this](QJsonDocument& jdoc) {
				Tmp(TWQEVENT::TWEET_ADDED, jdoc.object(), tr("tweet sended."));
			}, TWAPI::TW_UPDATE, pm);
			break;
		case USEROP::TWEET_REMOVE: {
			auto id = value.toULongLong();
			sgNet.restAPI([this,id](QJsonDocument& jdoc) {
				Tmp(TWQEVENT::TWEET_REMOVED, id, tr("tweet removed"));
				sgTweet.remInfo(id);
			}, TWAPI::TW_DESTROY, ParamMap(), id);
			break; }
		case USEROP::TWEET_OFCRT: {
			auto id = value.toULongLong();
			sgNet.restAPI([this,id](QJsonDocument& jdoc){
				auto jobj = jdoc.object();
				// 新しい(RT専用)ツイートを登録
				auto tid = JtoInt(jobj, JSONKWD::TWEET::IdStr);
				sgTweet.setInfo(tid, jobj);
				// RT元のツイートを更新してからTLに通知
				sgTweet.remInfo(id);
				sgTweet.getInfo<TwqTweet::P_Full>(id, _spVoid, [this,id,&jobj](const TwqTweet::P_Full& pf) {
					Tmp(TWQEVENT::TWEET_MODIFIED, id, tr("retweet done"));
				});
			}, TWAPI::TW_RETWEET, ParamMap(), id);
			break; }
		case USEROP::TWEET_FAVORITE: {
			auto id = value.toULongLong();
			sgNet.restAPI([this](QJsonDocument& jdoc){
				Tmp(TWQEVENT::TWEET_MODIFIED, jdoc.object(), tr("favorite done"));
			}, TWAPI::FV_CREATE, ParamMap(), id);
			break; }
		case USEROP::TWEET_CANCEL_OFCRT: {
			auto id = value.toULongLong();
			// RT元のIDを得る
			sgTweet.getInfo<TwqTweet::P_RT>(id, _spVoid, [this,id](const TwqTweet::P_RT& p) {
				auto rtid = p.reTweetID();
				if(rtid != INVALID_TweetID) {
					sgNet.restAPI([this,rtid,id](QJsonDocument& jdoc){
						Tmp(TWQEVENT::TWEET_REMOVED, rtid, tr("un-retweet done"));
						sgTweet.remInfo(rtid);

						sgTweet.remInfo(id);
						sgNet.restAPI([this,id](QJsonDocument& jdoc) {
							emit twqEvent(TWQEVENT::TWEET_MODIFIED, id);
						}, TWAPI::TW_SHOW, ParamMap(), id);
					}, TWAPI::TW_DESTROY, ParamMap(), rtid);
				} else
					showStatus(tr("internal error: invalid tweetID detected"));
			});
			break; }
		case USEROP::TWEET_UNFAVORITE: {
			auto id = value.toULongLong();
			sgNet.restAPI([this,id](QJsonDocument& jdoc){
				Tmp(TWQEVENT::TWEET_MODIFIED, jdoc.object(), tr("unfavorite done"));
			}, TWAPI::FV_DESTROY, ParamMap(), id);
			break; }
		case USEROP::TWEET_NOFCRT: {
			auto id = value.toULongLong();
			sgTweet.getInfo<TwqTweet::P_User>(id, _spVoid, [this](const TwqTweet::P_User& p) {
				sgUser.getInfo<TwqUser::P_Name>(p.userID(), _spVoid, [&p,this](const TwqUser::P_Name& pu) {
					_openTweetWindow("", QString(" RT @") + pu.screenName() + ' ' + p.text().left(90));
				});
			});
			break; }
		case USEROP::TWEET_QT: {
			auto id = value.toULongLong();
			sgTweet.getInfo<TwqTweet::P_User>(id, _spVoid, [this](const TwqTweet::P_User& p) {
				sgUser.getInfo<TwqUser::P_Name>(p.userID(), _spVoid, [&p,this](const TwqUser::P_Name& pu) {
					_openTweetWindow("", QString(" QT @") + pu.screenName() + ' ' + p.text().left(90));
				});
			});
			break; }
		case USEROP::TWEET_REPLY: {
			auto id = value.toULongLong();
			sgTweet.getInfo<TwqTweet::P_User>(id, _spVoid, [this](const TwqTweet::P_User& p) {
				sgUser.getInfo<TwqUser::P_Name>(p.userID(), _spVoid, [this](const TwqUser::P_Name& pu) {
					_openTweetWindow(QString('@') + pu.screenName() + ' ', "");
				});
			});
			break; }
		case USEROP::USER_REPLY: {
			auto id = value.toULongLong();
			sgUser.getInfo<TwqUser::P_Prop>(id, _spVoid, [this](const TwqUser::P_Prop& p) {
				_openTweetWindow(QString('@') + p.screenName() + ' ', "");
			});
			break; }
		case USEROP::USER_FOLLOW: {
			auto id = value.toULongLong();
			pm[TWAPIKWD::FR_CREATE::UserId] = QString("%1").arg(id);
			sgNet.restAPI([this,id](QJsonDocument& jdoc) {
				showStatus(tr("user %1 followed.").arg(jdoc.object().find(JSONKWD::USER::Name).value().toString()));
				emit twqEvent(TWQEVENT::USER_FOLLOW, id);
			}, TWAPI::FR_CREATE, pm);
			break; }
		case USEROP::USER_UNFOLLOW: {
			auto id = value.toULongLong();
			pm[TWAPIKWD::FR_DESTROY::UserId] = QString("%1").arg(id);
			sgNet.restAPI([this,id](QJsonDocument& jdoc){
				showStatus(tr("user %1 unfollowed.").arg(jdoc.object().find(JSONKWD::USER::Name).value().toString()));
				emit twqEvent(TWQEVENT::USER_UNFOLLOW, id);
			}, TWAPI::FR_DESTROY, pm);
			break; }
		case USEROP::USER_BLOCK: {
			auto id = value.toULongLong();
			pm[TWAPIKWD::BL_EXISTS::UserId] = QString("%1").arg(id);
			sgNet.restAPI([this,id](QJsonDocument& jdoc) {
				showStatus(tr("user %1 blocked.").arg(jdoc.object().find(JSONKWD::USER::Name).value().toString()));
				emit twqEvent(TWQEVENT::USER_UNFOLLOW, id);
			}, TWAPI::BL_CREATE, pm);
			break; }
		default:
			showStatus(tr("unknown user operation %1").arg((int)op));
	}
}
void TwqMain::showAPILimit() {
	const TwqNet::APILimit& a = sgNet.getApiLimit();
	QMessageBox::information(this, tr("API Limit"),
		QString(tr("remaining: %1\nreset time in seconds: %2\nhourly limit: %3\nreset time: %4"))
							 .arg(a.remainHits)
							 .arg(a.resetInSec)
							 .arg(a.hourlyLimit)
							 .arg(a.resetTime.toString()));
}
void TwqMain::updateAPILimit(const TwqNet::APILimit& api) {
	_lbAPILimit->setText(tr("remain:%1/%2").arg(api.remainHits).arg(api.p_remainHits));
}
void TwqMain::_addTL(TwqTL *tl, bool bRefl) {
	tl->show();
	centralWidget()->layout()->addWidget(tl);
	if(bRefl)
		tl->refreshPage();

	connect(tl, SIGNAL(status(QString)), this, SLOT(showStatus(QString)));
	connect(tl, SIGNAL(userOp(USEROP,QVariant)), this, SLOT(userOp(USEROP,QVariant)));
	connect(this, SIGNAL(twqEvent(TWQEVENT,QVariant)), tl, SLOT(twqEvent(TWQEVENT,QVariant)));
}
void TwqMain::addTLSearch() {
	DlgAddSearch dlg;
	DlgAddSearch::Result r;
	if(dlg.exec(r))
		_addTL(new TwqSearchTL(r.keyword, r.typekwd, this), false);
}
void TwqMain::addTLHome() {
	_addTL(new TwqHomeTL(this), true);
}
void TwqMain::addTLMentions() {
	_addTL(new TwqMentionsTL(this), true);
}

void TwqMain::quit() {
	qApp->quit();
}
#include <QLabel>
void TwqMain::authResult(bool bSuccess, const QString& err) {
	if(bSuccess) {
		showStatus(tr("authentication succeeded"));
	} else {
		// エラーメッセージを表示
		showStatus(err);
	}
}
void TwqMain::stateChanged(TwqNet::TOKEN state) {
	// タイトル表示を切り替える
	if(state == TwqNet::TOKEN::OK) {
		_action[(int)ACTION::APP_AUTH]->setEnabled(false);
		setWindowTitle(tr("TwilveQt - [authenticated]"));
	} else {
		_action[(int)ACTION::APP_AUTH]->setEnabled(true);
		setWindowTitle(tr("TwilveQt - [not authenticated]"));
	}
}
