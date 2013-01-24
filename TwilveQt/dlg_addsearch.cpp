#include "dlg_addsearch.h"
#include "ui_dlg_addsearch.h"
DlgAddSearch::DlgAddSearch(): _ui(new Ui::DlgAddSearch) {
	_ui->setupUi(this);
}

namespace {
    const static char
        *MixedType = "mixed",
        *PopularType = "popular",
        *RecentType = "recent";
}
bool DlgAddSearch::exec(Result& r) {
	if(QDialog::exec() == QDialog::Accepted) {
        r.keyword = _ui->searchText->text();
        if(_ui->typeMixed->isChecked())
            r.typekwd = MixedType;
        else if(_ui->typePopular->isChecked())
            r.typekwd = PopularType;
        else
            r.typekwd = RecentType;
                    
        return true;
	}
    return false;
}
