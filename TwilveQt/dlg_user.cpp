#include <QVariant>
#include "dlg_user.h"
#include "ui_dlg_user.h"
#include "json_kwd.h"
#include "twquser.h"
#include "twqMain.h"

DlgUser::DlgUser(QWidget *parent) :
	QWidget(parent),
	_ui(new Ui::DlgUser),
	_spVoid(new int)
{
	_ui->setupUi(this);
	_ui->tabTweet->setShowClose(false);
}

void DlgUser::showUser(UserID id) {
	_id = id;
	sgUser.getInfo<TwqUser::P_Picture, TwqUser::P_Prop>(id, _spVoid, [this,id](const TwqUser::P_Picture& p, const TwqUser::P_Prop& pp) {
		// ダイアログに色々設定する
		_ui->lbIcon->setPixmap(*p.profileIcon());
		_ui->lbName->setText(p.name());
		_ui->lbScName->setText(p.screenName());
		_ui->lbDescription->setText(pp.description());
		_ui->lbNTweets->setText(QString("%1").arg(pp.nTweet()));
		_ui->lbNFollow->setText(QString("%1").arg(pp.nFollow()));
		_ui->lbNFollower->setText(QString("%1").arg(pp.nFollower()));

		// ユーザのTLを読み込み
		_ui->tabTweet->showTL(id);
		_ui->tabTweet->refreshPage();

		// 対象ユーザと自分の関係を読み取る
		ParamMap pm;
		pm[TWAPIKWD::FR_SHOW::SourceId] = QString("%1").arg(sgNet.getMyID());
		pm[TWAPIKWD::FR_SHOW::TargetId] = QString("%1").arg(id);
		if(sgNet.getMyID() == id) {
		} else {
			sgNet.restAPI([id, this](QJsonDocument& jdoc) {
				QJsonObject jobj = jdoc.object();
				jobj = jobj.find("relationship").value().toObject();
				jobj = jobj.find("source").value().toObject();
				bool bF = jobj.find("following").value().toBool(),
					bFD = jobj.find("followed_by").value().toBool();

				_ui->btnBlock->setEnabled(bFD);
				_ui->btnFollow->setEnabled(!bF);
				_ui->btnUnfollow->setEnabled(bF);

				connect(_ui->btnFollow, SIGNAL(clicked()), this, SLOT(_makeFollow()));
				connect(_ui->btnUnfollow, SIGNAL(clicked()), this, SLOT(_makeUnFollow()));
				connect(_ui->btnBlock, SIGNAL(clicked()), this, SLOT(_makeBlock()));
			}, TWAPI::FR_SHOW, pm);
		}
		connect(_ui->tabTweet, SIGNAL(userOp(USEROP,QVariant)), this, SIGNAL(userOp(USEROP,QVariant)));
	});
}

void DlgUser::_makeFollow() {
	emit userOp(USEROP::USER_FOLLOW, QVariant(_id));
}
void DlgUser::_makeUnFollow() {
	emit userOp(USEROP::USER_UNFOLLOW, QVariant(_id));
}
void DlgUser::_makeBlock() {
	emit userOp(USEROP::USER_BLOCK, QVariant(_id));
}

void DlgUser::twqEvent(TWQEVENT e, const QVariant& param) {
	auto fnBtn = [this](bool bF) {
		// ボタンの更新
		_ui->btnUnfollow->setEnabled(bF);
		_ui->btnFollow->setEnabled(!bF);
	};

	switch(e) {
		case TWQEVENT::USER_FOLLOW:
			if(param.toULongLong() == _id)
				fnBtn(true);
			break;
		case TWQEVENT::USER_UNFOLLOW:
			if(param.toULongLong() == _id)
				fnBtn(false);
			break;
		case TWQEVENT::USER_BLOCK:
			if(param.toULongLong() == _id)
				_ui->btnBlock->setEnabled(false);
			break;
		default:
			break;
	}
}
