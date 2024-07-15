// #include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <camlib.h>
#include <pthread.h>
#include <camlua.h>
#include <ui.h>
#include <app.h>
#include "desktop.h"

int luaopen_libuilua(lua_State *L);

void uiToast() {}

struct App {
	int is_opened;
	uiWindow *main_win;
	uiLabel *main_log;
	uiMultilineEntry *script_box;
	uiMultilineEntry *connect_entry;
}app = {0};

struct PtpRuntime *luaptp_get_runtime(lua_State *L) {
	return ptp_get();
}

void ui_send_text(char *key, char *value, ...) {
	if (!app.is_opened) {
		return;
	}
	if (!strcmp(key, "cam_name")) {
		char buffer[64];
		sprintf(buffer, "Fudge - %s", value);
		uiWindowSetTitle(app.main_win, buffer);
	}
}

static void print_thread(char *buffer) {
	uiMultilineEntryAppend(app.connect_entry, buffer);
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	if (app.is_opened) {
		uiMultilineEntryAppend(app.connect_entry, buffer);
		//uiLabelSetText(app.main_log, buffer);
		uiQueueMain(print_thread, (void *)strdup(buffer));
	} else {
		puts(buffer);
	}
}

static char *read_file(char *filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) return NULL;
    
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

static int onShouldQuit(void *data)
{
	uiWindow *mainwin = uiWindow(data);
	uiControlDestroy(uiControl(mainwin));
	return 1;
}

static int modelNumColumns(uiTableModelHandler *mh, uiTableModel *m)
{
	return 3;
}

static uiTableValueType modelColumnType(uiTableModelHandler *mh, uiTableModel *m, int column)
{
	return uiTableValueTypeString;
}

static int modelNumRows(uiTableModelHandler *mh, uiTableModel *m)
{
	return 100;
}

static uiTableValue *modelCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column)
{
	switch (column) {
	case 0: return uiNewTableValueString("TEST.JPG");
	default: return uiNewTableValueString("Date");
	}
}

static void modelSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *val)
{

}

static uiControl *files_tab() {

	static uiTableModelHandler mh = {
		.NumColumns = modelNumColumns,
		.ColumnType = modelColumnType,
		.NumRows = modelNumRows,
		.CellValue = modelCellValue,
		.SetCellValue = modelSetCellValue,
	};

	uiTableModel *m = uiNewTableModel(&mh);

	uiTableParams p = {
		.Model = m,
		.RowBackgroundColorModelColumn = 3,
	};

	uiTable *t = uiNewTable(&p);

	uiTableAppendTextColumn(t, "Filename",
	0, uiTableModelColumnNeverEditable, NULL);

	uiTableAppendTextColumn(t, "Last Modified",
	1, uiTableModelColumnNeverEditable, NULL);

	return uiControl(t);
}

extern void *fudge_backup_test(void *arg);

static void usb_connect(uiButton *btn, void *data) {
	pthread_t thread;

	pthread_create(&thread, 0, fudge_backup_test, NULL);
}

int cam_lua_setup(lua_State *L) {
	luaL_requiref(L, "ui", luaopen_libuilua, 1);
	return 0;
}

static void script_run(uiMenuItem *sender, uiWindow *window, void *senderData) {
	char *file = uiMultilineEntryText(app.script_box);
	if (cam_run_lua_script_async(file) < 0) {
		uiToast(cam_lua_get_error());
	}

	uiFreeText(file);
}

uiControl *editor_tab() {
	app.script_box = uiNewNonWrappingMultilineEntry();
	uiMultilineEntrySetText(app.script_box, read_file("../app/src/main/assets/script.lua"));
	return uiControl(app.script_box);
}

uiControl *connect_tab() {
	uiMultilineEntry *entry = uiNewMultilineEntry();
	uiEntrySetReadOnly(entry, 1);

	app.connect_entry = entry;

	return (uiControl *)entry;
}

static uiControl *fudge_screen() {
	uiBox *box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	uiGroup *connectivity = uiNewGroup("Connect");
	{
		uiBox *hbox = uiNewVerticalBox();
		uiButton *btn = uiNewButton("Connect to USB");
		uiBoxAppend(hbox, uiControl(btn), 1);
		uiButtonOnClicked(btn, usb_connect, NULL);
		uiBoxAppend(hbox, uiControl(uiNewButton("Connect to WiFi")), 1);
		uiGroupSetChild(connectivity, uiControl(hbox));
	}
	uiBoxAppend(box, uiControl(connectivity), 0);

	uiGroup *discovery = uiNewGroup("Searching for a camera...");
	{
		uiBox *inner = uiNewVerticalBox();

		uiBoxAppend(inner, uiControl(uiNewLabel("- PC AutoSave")), 0);
		uiBoxAppend(inner, uiControl(uiNewLabel("- Wireless Tether")), 0);

		uiProgressBar *bar = uiNewProgressBar();
		uiProgressBarSetValue(bar, -1);
		uiBoxAppend(inner, uiControl(bar), 0);

		uiGroupSetChild(discovery, uiControl(inner));
	}
	uiBoxAppend(box, uiControl(discovery), 0);

    uiTab *tab = uiNewTab();
    uiTabAppend(tab, "Files", files_tab());
    uiTabAppend(tab, "Lua Editor", editor_tab());
    uiTabAppend(tab, "Recipes", files_tab());
    uiTabAppend(tab, "Developer", files_tab());
	uiBoxAppend(box, uiControl(tab), 1);
	uiControlHide(uiControl(tab));

	app.main_log = uiNewLabel("Doing things...");
	uiBoxAppend(box, uiControl(app.main_log), 0);

	return (uiControl *)box;
}

int fudge_main_ui(void)
{
	uiInitOptions options;
	const char *err;
	uiTab *tab;

	memset(&options, 0, sizeof (uiInitOptions));
	err = uiInit(&options);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s", err);
		uiFreeInitError(err);
		return 1;
	}

	{
		uiMenuItem *item;
		uiMenu *menu = uiNewMenu("Script");
		item = uiMenuAppendItem(menu, "Run");
		uiMenuItemOnClicked(item, script_run, NULL);
		item = uiMenuAppendItem(menu, "Open");
		item = uiMenuAppendItem(menu, "Save");
		item = uiMenuAppendPreferencesItem(menu);
		item = uiMenuAppendQuitItem(menu);

		menu = uiNewMenu("Connect");
		item = uiMenuAppendItem(menu, "USB");
		uiMenuItemOnClicked(item, usb_connect, NULL);
		item = uiMenuAppendItem(menu, "WiFi");
	}

	app.main_win = uiNewWindow("Fudge", 640, 480, 1);
	uiWindowOnClosing(app.main_win, onClosing, NULL);
	uiOnShouldQuit(onShouldQuit, app.main_win);
	uiWindowSetMargined(app.main_win, 1);

	uiWindowSetChild(app.main_win, fudge_screen());

	uiControlShow(uiControl(app.main_win));
	uiMain();
	return 0;
}
