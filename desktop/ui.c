#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ui.h>

static uiWindow *main_win = NULL;
static uiLabel *main_log = NULL;

void ui_send_text(char *key, char *value) {
	if (!strcmp(key, "cam_name")) {
		char buffer[64];
		sprintf("Fudge - %s", value);
		uiWindowSetTitle(main_win, buffer);
	}
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	uiLabelSetText(main_log, buffer);
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

uiControl *download_photos_tab() {
	return uiNewVerticalBox();
}

uiControl *fudge_screen() {
	uiBox *box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiBox *hbox = uiNewHorizontalBox();
	uiBoxAppend(box, uiControl(hbox), 0);
//	uiBoxSetPadded(hbox, 1);

	uiBoxAppend(hbox, uiControl(uiNewButton("Connect to USB")), 0);
	//uiBoxAppend(hbox, uiControl(uiNewButton("Connect to WiFi")), 0);

	uiMultilineEntry *editor = uiNewNonWrappingMultilineEntry();

    uiTab *tab = uiNewTab();
    uiTabAppend(tab, "Lua Editor", uiControl(editor));
    uiTabAppend(tab, "Photos", download_photos_tab());
    uiTabAppend(tab, "Recipes", download_photos_tab());
    uiTabAppend(tab, "Developer", download_photos_tab());
	uiBoxAppend(box, uiControl(tab), 1);

	main_log = uiNewLabel("Doing things...");
	uiBoxAppend(box, uiControl(main_log), 0);

	return box;
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
		uiMenu *menu = uiNewMenu("File");
		uiMenuItem *item = uiMenuAppendItem(menu, "Open");
		//uiMenuItemOnClicked(item, openClicked, NULL);
		item = uiMenuAppendItem(menu, "Save");
		//uiMenuItemOnClicked(item, saveClicked, NULL);
		item = uiMenuAppendPreferencesItem(menu);
		item = uiMenuAppendQuitItem(menu);

		menu = uiNewMenu("Connect");
		item = uiMenuAppendItem(menu, "USB");
		item = uiMenuAppendItem(menu, "WiFi");
	}

	uiWindow *main_win = uiNewWindow("Fudge", 640, 480, 1);
	uiWindowOnClosing(main_win, onClosing, NULL);
	uiOnShouldQuit(onShouldQuit, main_win);
	uiWindowSetMargined(main_win, 1);

	uiWindowSetChild(main_win, fudge_screen());

	uiControlShow(uiControl(main_win));
	uiMain();
	return 0;
}
