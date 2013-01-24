#ifndef TWQTLVARIANT_H
#define TWQTLVARIANT_H

#include "twqtl.h"
#include "twqNet.h"
//! キーワード検索
class TwqSearchTL : public TwqTL {
	Q_OBJECT
	TwqNet::RestCB		_cbP;
	QString				_keyword, _typekwd;
	ParamMap			_paramRefl,
						_paramNext;
	public slots:
		void searchFor(const QString& keyword, const QString& tkw);
	public:
		explicit TwqSearchTL(const QString& keyword, const QString& tkw, QWidget* p=0);
		void readNextPage();
		void refreshPage();
};

//! HomeTL
class TwqHomeTL : public TwqTL {
	Q_OBJECT
	protected:
		TwqNet::RestCB	_cbP;
		void _refreshPage(TWAPI::INDEX idx, const ParamMap& param);
		void _readNextPage(TWAPI::INDEX idx, const ParamMap& param);
	public:
		explicit TwqHomeTL(QWidget* p=nullptr);
		void readNextPage();
		void refreshPage();
};

//! UserTL
class TwqUserTL : public TwqHomeTL {
	Q_OBJECT
	UserID		_userID;
	public:
		explicit TwqUserTL(QWidget* p=nullptr);
		void showTL(UserID id);
		void readNextPage();
		void refreshPage();
};

//! MentionsTL
class TwqMentionsTL : public TwqHomeTL {
	Q_OBJECT
	public:
		explicit TwqMentionsTL(QWidget* p=nullptr);
		void readNextPage();
		void refreshPage();
};

// FriendsList
// FollowersList

#endif // TWQTLVARIANT_H
