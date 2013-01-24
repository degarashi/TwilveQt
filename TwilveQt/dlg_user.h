#ifndef DLGUSER_H
#define DLGUSER_H

#include <QWidget>
#include "common.h"

namespace Ui {
	class DlgUser;
}
class DlgUser : public QWidget {
	Q_OBJECT
	std::shared_ptr<Ui::DlgUser> _ui;
	SPVoid		_spVoid;
	UserID		_id;

	public:
		explicit DlgUser(QWidget *parent = 0);
		void showUser(UserID id);
	private slots:
		void _makeFollow();
		void _makeUnFollow();
		void _makeBlock();
	public slots:
		void twqEvent(TWQEVENT e, const QVariant& param);
	signals:
		void userOp(USEROP op, const QVariant& param);
};

#endif // DLGUSER_H
