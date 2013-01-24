#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include "twqMain.h"
#include "oauth.h"
#include "twqNet.h"
#include "twquser.h"
#include "twqtweet.h"
#include "twqresource.h"
#include <QFile>
#include <QDir>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	// 翻訳ファイルをセットする
	QString c_code = QLocale::system().name().split("_")[0];
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf8"));
	// :Qtライブラリ
	QTranslator trQ, trMe;
	trQ.load("qt_" + c_code,
			 QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&trQ);
	// :MyApp
	trMe.load("../TwilveQt/tr_" + c_code);
	a.installTranslator(&trMe);

	std::unique_ptr<TwqNet> pNet(new TwqNet(nullptr));
	std::unique_ptr<TwqUser> pUser(new TwqUser(nullptr));
	std::unique_ptr<TwqTweet> pTweet(new TwqTweet(nullptr));
	std::unique_ptr<TwqResource> pRes(new TwqResource);

	TwqMain w;
	w.show();
	try {
		return a.exec();
	} catch(const ExceptionBase& e) {
		QMessageBox::warning(&w, QApplication::tr("unknown error"),
							 QApplication::tr("unhandled exception throwed\n%1").arg(e.whatQs()));
	}
	return 1;
}
