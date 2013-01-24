#include "validation.h"
#include "ui_validation.h"

validation::validation(QWidget *parent) :
	QDialog(parent),
	_ui(new Ui_validation)
{
	_ui->setupUi(this);
}

QString validation::execValidation() {
	if(QDialog::exec() == QDialog::Accepted)
		return _ui->idbox->text();
	return QString();
}
