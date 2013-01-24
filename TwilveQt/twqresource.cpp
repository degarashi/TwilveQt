#include <QApplication>
#include "twqresource.h"

const char* TwqResource::cs_icon[] = {
	"../image/icon_fav.png",
	"../image/icon_rt.png"
};
const char* TwqResource::cs_mov[] = {
	"../image/icon_loading.gif",
};
const QColor TwqResource::cs_color[] = {
	QColor(255,210,128),
	QColor(128,255,128),
	QColor(160,160,240),
	QColor(255,255,255)
};

TwqResource::TwqResource() {
	auto path = QApplication::applicationDirPath() + '/';
	for(int i=0 ; i<static_cast<int>(_countof(cs_icon)) ; i++)
		_icon[i] = QIcon(path + cs_icon[i]);
	for(int i=0 ; i<static_cast<int>(_countof(cs_mov)) ; i++) {
		_mov[i].~QMovie();
		new(_mov+i) QMovie(path + cs_mov[i]);
	}
}

const QColor& TwqResource::getColor(COLOR c) const {
	return cs_color[(int)c];
}
const QIcon& TwqResource::getIcon(ICON ic) const {
	return _icon[(int)ic];
}
QMovie& TwqResource::getMovie(MOVIE m) {
	return _mov[(int)m];
}