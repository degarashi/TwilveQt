#include <QTextDocument>
#include <QPushButton>
#include "dlg_tweet.h"
#include "ui_dlg_tweet.h"

DlgTweet::DlgTweet(QWidget *parent) :
	QDialog(parent),
	_ui(new Ui::DlgTweet)
{
	_ui->setupUi(this);

	// 文字数カウント
	connect(_ui->tweetText, SIGNAL(textChanged()), this, SLOT(_countChar()));

	_countChar();
}

QString DlgTweet::exec(const QString& pre, const QString& post) {
	_preStr = pre;
	_postStr = post;

	int res = QDialog::exec();
	if(res == QDialog::Accepted) {
		Q_ASSERT(_ui->tweetText->toPlainText().length() < MAX_TWEET);
		return _ui->tweetText->toPlainText();
	}
	return QString();
}
void DlgTweet::showEvent(QShowEvent *e) {
	// 引数のテキストを追加
	_ui->tweetText->document()->setPlainText(_preStr + _postStr);
//    QTextDocument* doc = new QTextDocument(_preStr + _postStr);
//    QPlainTextDocumentLayout* lo = new QPlainTextDocumentLayout(doc);
//    doc->setDocumentLayout(lo);
//    _ui->tweetText->setDocument(doc);

	// カーソルをpreとpostの間へ移動
	QTextCursor c = _ui->tweetText->textCursor();
	c.setPosition(_preStr.length());
	_ui->tweetText->setTextCursor(c);
}

void DlgTweet::_countChar() {
	QString str = _ui->tweetText->toPlainText();
	QPushButton* btn = _ui->buttonBox->button(QDialogButtonBox::Ok);
	QLabel* lb = _ui->txtCount;
	QPalette pl = lb->palette();
	QColor colLb;
	// 文字制限超えてたら・・
	int sz = str.toUcs4().size();
	if(sz > MAX_TWEET) {
		// 赤文字にする
		colLb = QColor(255,0,0);
		// ボタン無効化
		btn->setEnabled(false);
	} else {
		colLb = QColor(0,0,0);
		btn->setEnabled(true);
	}
	pl.setColor(QPalette::WindowText, colLb);
	lb->setPalette(pl);

	lb->setText(QString("%1").arg(sz));
}
