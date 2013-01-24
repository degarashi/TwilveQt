#ifndef DLG_ADDSEARCH_H
#define DLG_ADDSEARCH_H
#include <memory>
#include <QDialog>

namespace Ui {
	class DlgAddSearch;
}
class DlgAddSearch : public QDialog {
	Q_OBJECT
	std::shared_ptr<Ui::DlgAddSearch>	_ui;

	public:
		struct Result {
			QString keyword, typekwd;
		};

		DlgAddSearch();
		bool exec(Result& r);
};

#endif // DLG_ADDSEARCH_H
