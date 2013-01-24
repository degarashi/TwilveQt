#ifndef TWQRESOURCE_H
#define TWQRESOURCE_H

#include <QIcon>
#include <QColor>
#include <QMovie>
#include "common.h"
#define sgRes TwqResource::_ref()
class TwqResource : public Singleton<TwqResource> {
	public:
		enum class COLOR {
			FAVORITE,
			RETWEET,
			SELECT_TEXT,
			SELECT_BG,
			NUM_COLOR
		};
		enum class ICON {
			FAVORITE,
			RETWEET,
			NUM_ICON
		};
		enum class MOVIE {
			LOADING,
			NUM_MOVIE
		};

	private:
		const static QColor cs_color[(int)COLOR::NUM_COLOR];
		const static char *cs_icon[(int)ICON::NUM_ICON],
							*cs_mov[(int)MOVIE::NUM_MOVIE];
		QIcon	_icon[(int)ICON::NUM_ICON];
		QMovie	_mov[(int)MOVIE::NUM_MOVIE];

	public:
		TwqResource();
		const QColor& getColor(COLOR c) const;
		const QIcon& getIcon(ICON ic) const;
		QMovie& getMovie(MOVIE m);
};

#endif // TWQRESOURCE_H
