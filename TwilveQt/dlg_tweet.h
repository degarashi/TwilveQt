#ifndef DLGTWEET_H
#define DLGTWEET_H

#include <QDialog>
#include <memory>
namespace Ui {
	class DlgTweet;
}
class DlgTweet : public QDialog {
	Q_OBJECT
	const static int MAX_TWEET = 140;
	QString     _preStr,
				_postStr;
	//! ウィンドウクラス
	/*! 本当はunique_ptrで間に合うのだが依存ヘッダが増えてしまう都合上shared_ptrにしている */
	std::shared_ptr<Ui::DlgTweet>	_ui;

	private slots:
		void _countChar();
		void showEvent(QShowEvent *e);
	public:
		explicit DlgTweet(QWidget *parent = 0);

		// s = 始めにセットする文字列
		QString exec(const QString& pre="", const QString& post="");
};

#endif // DLGTWEET_H
