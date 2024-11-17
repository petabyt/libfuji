// 2024 Fudge desktop frontend
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <camlib.h>
#include <pthread.h>
#include <ui.h>
#include "safety.h"
#include <app.h>
#include "desktop.h"

void uiToast(const char *format, ...) {}

struct App {
	int is_opened;
	uiWindow *main_win;
	uiMultilineEntry *main_log;
	uiMultilineEntry *script_box;
	uiMultilineEntry *connect_entry;
	uiLabel *bottom_status;
	uiTab *right_tab;
}app = {0};

void app_send_cam_name(const char *name) {
	if (!app.is_opened) {
		return;
	}

	if (name) {
		char buffer[64];
		sprintf(buffer, "Fudge - Connected to %s", name);
		uiWindowSetTitle(app.main_win, buffer);
		sprintf(buffer, "Connected to %s", name);
		uiLabelSetText(app.bottom_status, buffer);
	} else {
		uiWindowSetTitle(app.main_win, "Fudge");		
		uiLabelSetText(app.bottom_status, "Not connected to any camera");
	}
}

void fuji_discovery_update_progress(void *arg, int progress) {
	if (!app.is_opened) return;
}

void app_downloading_file(const struct PtpObjectInfo *oi) {
	if (!app.is_opened) return;
	// ...
}

void app_downloaded_file(const struct PtpObjectInfo *oi, const char *path) {
	if (!app.is_opened) return;
	// ...
}

void app_increment_progress_bar(int read) {
	if (!app.is_opened) return;
	// ...
}

void app_report_download_speed(long time, size_t size) {
	if (!app.is_opened) return;
	// ...
}

void tester_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("LOG: %s\n", buffer);
}

void tester_fail(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("FAIL: %s\n", buffer);
}

static void clear_thread(void *arg) {
	uiMultilineEntrySetText(app.main_log, "");
}

static void print_thread(void *buffer) {
	uiMultilineEntryAppend(app.main_log, (const char *)buffer);
	free(buffer);
	uiMultilineEntryAppend(app.main_log, "\n");
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	if (app.is_opened) {
		uiQueueMain(print_thread, (void *)strdup(buffer));
	} else {
		puts(buffer);
	}
}

void app_log_clear(void) {
	uiQueueMain(clear_thread, NULL);
}

static char *read_file(const char *filename) {
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
	fudge_disconnect_all();
	uiQuit();
	return 1;
}

static int onShouldQuit(void *data)
{
	uiWindow *mainwin = uiWindow(data);
	uiControlDestroy(uiControl(mainwin));
	return 1;
}

static uiControl *files_tab(void) {
	return uiControl(uiNewLabel("TODO..."));
}

static uiControl *general_tab(void) {
	uiBox *box = uiNewVerticalBox();
	uiBoxAppend(box, uiControl(uiNewLabel("Hello")), 0);

	{
		uiGroup *g = uiNewGroup("Camera Settings");
		uiButton *btn = uiNewButton("Backup settings");
		uiGroupSetChild(g, uiControl(btn));
		uiBoxAppend(box, uiControl(g), 0);
	}

	return uiControl(box);
}

static void usb_connect(uiButton *btn, void *data) {
	pthread_t thread;
	pthread_create(&thread, 0, fudge_usb_connect_thread, NULL);
}

static void usb_connect_menu(uiMenuItem *i, uiWindow *w, void *data) {
	pthread_t thread;
	pthread_create(&thread, 0, fudge_usb_connect_thread, NULL);
}

static void script_run(uiMenuItem *sender, uiWindow *window, void *senderData) {
	char *file = uiMultilineEntryText(app.script_box);
	if (cam_run_lua_script_async(file) < 0) {
		uiToast(cam_lua_get_error());
	}

	uiFreeText(file);
}

void app_update_connected_status_(void *arg) {
	uiTab *tab = app.right_tab;
	if ((uintptr_t)arg) {
		uiTabInsertAt(tab, "General", 0, general_tab());
		uiTabSetSelected(tab, 0);
	} else {
		uiTabSetSelected(tab, 1);
		uiTabDelete(tab, 0);
		app_send_cam_name(NULL);
	}
}
void app_update_connected_status(int connected) {
	uiQueueMain(app_update_connected_status_, (void *)(uintptr_t)connected);
}

uiControl *editor_tab(void) {
	char *file = read_file("../android/app/src/main/assets/script.lua");
	if (file == NULL) file = "null";
	app.script_box = uiNewNonWrappingMultilineEntry();
	uiMultilineEntrySetText(app.script_box, file);
	return uiControl(app.script_box);
}

uiControl *connect_tab(void) {
	uiMultilineEntry *entry = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(entry, 1);

	app.connect_entry = entry;

	return (uiControl *)entry;
}

uiControl *create_discovery_ui(void) {
	uiGroup *discovery = uiNewGroup("Searching for a camera...");
	uiBox *inner = uiNewVerticalBox();

	uiBoxAppend(inner, uiControl(uiNewLabel("- PC AutoSave")), 0);
	uiBoxAppend(inner, uiControl(uiNewLabel("- Wireless Tether")), 0);

	uiProgressBar *bar = uiNewProgressBar();
	uiProgressBarSetValue(bar, -1);
	uiBoxAppend(inner, uiControl(bar), 0);

	uiGroupSetChild(discovery, uiControl(inner));
	return uiControl(discovery);
}

uiControl *about_tab(void) {
	uiBox *box = uiNewVerticalBox();
	//uiBoxSetPadded(box, 1);

	{
		uiGroup *g = uiNewGroup("Fudge");
		uiMultilineEntry *text = uiNewMultilineEntry();
		uiMultilineEntrySetReadOnly(text, 1);
		uiMultilineEntrySetText(text,
			"Connect to a camera to get started.\n"
			"For questions, feature requests, or bug reports, shoot me an email or file an issue on the Github page.\n"
			"- brikbusters@gmail.com\n"
			"- https://github.com/petabyt/fudge\n"
		);
		uiGroupSetChild(g, uiControl(text));
		uiBoxAppend(box, uiControl(g), 1);
	}

	{
		uiGroup *g = uiNewGroup("Credits");
		uiMultilineEntry *text = uiNewMultilineEntry();
		uiMultilineEntrySetReadOnly(text, 1);
		uiMultilineEntrySetText(text,
			"Created by Daniel Cook (danielc.dev)\n"
			"Libraries:\n"
			"- https://github.com/libui-ng/libui-ng (MIT License)\n"
			"- https://github.com/petabyt/libwpd\n"
			"- https://github.com/petabyt/camlib\n"
			"- Lua 5.3 (MIT License)\n"
		);
		uiGroupSetChild(g, uiControl(text));
		uiBoxAppend(box, uiControl(g), 1);
	}

	return uiControl(box);
}

static uiControl *fudge_screen(void) {
	uiBox *box = uiNewHorizontalBox();
	uiBoxSetPadded(box, 1);

	uiBox *left = uiNewVerticalBox();
	//uiBoxSetPadded(left, 1);

	uiGroup *connectivity = uiNewGroup("Connect");
	{
		uiBox *hbox = uiNewVerticalBox();
		//uiBoxSetPadded(hbox, 1);
		uiButton *btn = uiNewButton("USB");
		uiControlSetTooltip(uiControl(btn), "Find a camera over USB and connect");
		uiButtonOnClicked(btn, usb_connect, NULL);
		uiBoxAppend(hbox, uiControl(btn), 1);
		uiBoxAppend(hbox, uiControl(uiNewButton("WiFi")), 1);
		uiBoxAppend(hbox, uiControl(uiNewButton("Search for camera")), 1);
		uiGroupSetChild(connectivity, uiControl(hbox));
	}
	uiBoxAppend(left, uiControl(connectivity), 0);

	app.main_log = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(app.main_log, 1);
	uiBoxAppend(left, uiControl(app.main_log), 1);

	uiBoxAppend(box, uiControl(left), 1);

	{
		uiBox *vbox = uiNewVerticalBox();
		uiTab *tab = uiNewTab();
		app.right_tab = tab;
		//uiTabAppend(tab, "Files", files_tab());
		//uiTabAppend(tab, "Lua Editor", editor_tab());
		//uiTabAppend(tab, "Recipes", files_tab());
		uiTabAppend(tab, "About", about_tab());
		uiBoxAppend(vbox, uiControl(tab), 1);
		uiLabel *lbl = uiNewLabel("Not connected to any camera");
		app.bottom_status = lbl;
		uiBoxAppend(vbox, uiControl(lbl), 0);
		uiBoxAppend(box, uiControl(vbox), 1);
	}

	return (uiControl *)box;
}

int multiple_cameras_menu(void) {

	uiWindow *w = uiNewWindow("Dialog", 500, 100, 0);
	uiWindowSetMargined(w, 1);

	uiBox *screen = uiNewVerticalBox();

	uiBoxAppend(screen, uiControl(uiNewLabel("Choose a camera to connect to")), 1);

	uiButton *btn = uiNewButton("Fujfilm X-T4");
	uiBoxAppend(screen, uiControl(btn), 1);

	btn = uiNewButton("Fujifilm X-A2");
	uiBoxAppend(screen, uiControl(btn), 1);

	btn = uiNewButton("Cancel");
	uiBoxAppend(screen, uiControl(btn), 1);

	uiWindowSetChild(w, uiControl(screen));
	uiControlShow(uiControl(w));

	return 0;
}

int fudge_main_ui(void) {
	app.is_opened = 1;
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
		uiMenuItemOnClicked(item, usb_connect_menu, NULL);
		item = uiMenuAppendItem(menu, "WiFi");
	}

	app.main_win = uiNewWindow("Fudge", 1000, 500, 1);
	uiWindowOnClosing(app.main_win, onClosing, NULL);
	uiOnShouldQuit(onShouldQuit, app.main_win);
	uiWindowSetMargined(app.main_win, 1);

	uiWindowSetChild(app.main_win, fudge_screen());
	uiControlShow(uiControl(app.main_win));

	app_print("Fudge desktop pre-release");

	uiMain();
	return 0;
}
