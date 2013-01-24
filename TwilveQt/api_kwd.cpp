#include "api_kwd.h"

bool TWAPI::TWApiInfo::isGET() const {
	return _bGET;
}
int TWAPI::TWApiInfo::requireAuth() const {
	return _iAuth;
}
const QString& TWAPI::TWApiInfo::host() const {
	return _host;
}
QString TWAPI::TWApiInfo::path() const {
	return _path;
}
QString TWAPI::TWApiInfo::path(const QString& str) const {
	return _path.arg(str);
}
QString TWAPI::TWApiInfo::path(int64_t num) const {
	return _path.arg(num);
}
