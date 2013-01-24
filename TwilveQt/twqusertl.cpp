#include "twqtlvariant.h"
#include "api_kwd.h"
#include "twqNet.h"

TwqUserTL::TwqUserTL(QWidget *p):
	TwqHomeTL(p),
	_userID(INVALID_UserID)
{
}

void TwqUserTL::showTL(UserID id) {
	_userID = id;
	repaint();
}

void TwqUserTL::refreshPage() {
	if(_userID != INVALID_UserID) {
		ParamMap pm;
		pm[TWAPIKWD::TL_USER::UserId] = QString("%1").arg(_userID);
		_refreshPage(TWAPI::TL_USER, pm);
	}
}
void TwqUserTL::readNextPage() {
	Q_ASSERT(_userID != INVALID_UserID);

	ParamMap param;
	param[TWAPIKWD::TL_USER::UserId] = QString("%1").arg(_userID);
	_readNextPage(TWAPI::TL_USER, param);
}
