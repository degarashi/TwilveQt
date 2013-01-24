#ifndef VALIDATION_H
#define VALIDATION_H

#include <QDialog>
#include <memory>
#include "ui_validation.h"

class validation : public QDialog {
	Q_OBJECT
	std::unique_ptr<Ui_validation>	_ui;

	public:
		explicit validation(QWidget *parent = 0);
		QString execValidation();
};

#endif // VALIDATION_H
