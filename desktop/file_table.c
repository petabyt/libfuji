#include <stdlib.h>
#include <ui.h>
#include <object.h>
#include <object.h>
#include <app.h>

struct MyTableModelHandler {
	uiTableModelHandler mh;
	struct PtpRuntime *r;
	struct PtpObjectCache *oc;
};

static inline struct MyTableModelHandler *my_handler(uiTableModelHandler *mh) {
	return (struct MyTableModelHandler *)mh;
}

static int modelNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
	return 3;
}

static uiTableValueType modelColumnType(uiTableModelHandler *mh, uiTableModel *m, int column)
{
	switch (column) {
	case 0:
		return uiTableValueTypeString;
	case 1:
		return uiTableValueTypeString;
	default:
		return uiTableValueTypeString;
	}
}

static int modelNumRows(uiTableModelHandler *mh, uiTableModel *m) {
	return ptp_object_service_length(
		my_handler(mh)->r,
		my_handler(mh)->oc
	);
}

static uiTableValue *modelCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column)
{
	struct MyTableModelHandler *h = my_handler(mh);

	if (column == 0) {
		struct PtpObjectInfo *oi = ptp_object_service_get_index(h->r, h->oc, row);
		return uiNewTableValueString(oi->filename);
	}

	return uiNewTableValueString("TODO");
}

static void modelSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *val) {
	// This list is not editable
}

uiControl *create_files_tab(struct PtpRuntime *r, struct PtpObjectCache *oc) {
	struct MyTableModelHandler *mh = (struct MyTableModelHandler *)malloc(sizeof(struct MyTableModelHandler));
	mh->mh.NumColumns = modelNumColumns;
	mh->mh.ColumnType = modelColumnType;
	mh->mh.NumRows = modelNumRows;
	mh->mh.CellValue = modelCellValue;
	mh->mh.SetCellValue = modelSetCellValue;
	mh->r = r;
	mh->oc = oc;

	uiTableModel *m = uiNewTableModel((uiTableModelHandler *)mh);

	uiTableParams p = {
		.Model = m,
		.RowBackgroundColorModelColumn = 3,
	};

	uiTable *t = uiNewTable(&p);

	uiTableAppendTextColumn(t, "Filename", 0, uiTableModelColumnNeverEditable, NULL);
	uiTableAppendTextColumn(t, "Last Modified", 1, uiTableModelColumnNeverEditable, NULL);

	return uiControl(t);
}
