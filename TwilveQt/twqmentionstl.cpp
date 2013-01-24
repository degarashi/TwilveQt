#include "twqtlvariant.h"
#include "twqNet.h"
#include "json_kwd.h"
#include "twqcell.h"

TwqMentionsTL::TwqMentionsTL(QWidget *p):
	TwqHomeTL(p)
{
	setTLName(tr("mentions"));
}

void TwqMentionsTL::refreshPage() {
	ParamMap pm;
	pm.insert(TWAPIKWD::TW_SHOW::IncludeMyRetweet, "true");
	_refreshPage(TWAPI::TL_MENTIONS, pm);
}
void TwqMentionsTL::readNextPage() {
	ParamMap pm;
	pm.insert(TWAPIKWD::TW_SHOW::IncludeMyRetweet, "true");
	_readNextPage(TWAPI::TL_MENTIONS, pm);
}
