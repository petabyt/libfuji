package libui;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.Toast;
import android.app.ActionBar;

public class LibUI {
    public static Context ctx = null;
    public static ActionBar actionBar = null;

    // uiWindow (popup) background drawable style resource
    public static int popupDrawableResource = 0;

    // Background drawable resource for buttons
    public static int buttonBackgroundResource = 0;

    public static Boolean useActionBar = true;

    public static void init(Activity act) {
        ctx = (Context)act;
    }

    public static void start(Activity act) {
        init(act);
        initThiz(ctx);
    }

    // Common way of telling when activity is done loading
    private static void waitUntilActivityLoaded(Activity activity) {
        ViewTreeObserver viewTreeObserver = activity.getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                activity.getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                init();
            }
        });
    }

    private static void init() {
        if (useActionBar) {
            actionBar = ((Activity)ctx).getActionBar();
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    private static class MySelectListener implements AdapterView.OnItemSelectedListener {
        byte[] struct;
        public MySelectListener(byte[] struct) {
            this.struct = struct;
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            LibUI.callFunction(this.struct);
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {}
    }

    private static class MyOnClickListener implements View.OnClickListener {
        byte[] struct;
        public MyOnClickListener(byte[] struct) {
            this.struct = struct;
        }

        @Override
        public void onClick(View v) {
            LibUI.callFunction(struct);
        }
    }

    public static View form(String name) {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title = new TextView(ctx);
        title.setPadding(5, 5, 5, 5);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setText(name);
        title.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.WRAP_CONTENT
        ));

        layout.addView(title);

        return layout;
    }

    public static void formAppend(View form, String name, View child) {
        LinearLayout entry = new LinearLayout(ctx);
        entry.setOrientation(LinearLayout.HORIZONTAL);
        entry.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView entryName = new TextView(ctx);
        entryName.setPadding(20, 10, 20, 10);
        entryName.setText(name);
        entryName.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT
        ));
        entry.addView(entryName);

        entry.addView(child);

        ((LinearLayout)form).addView(entry);
    }

    @SuppressWarnings("deprecation")
    public static View tabLayout() {
        TabHost tabHost = new TabHost(ctx, null);
        tabHost.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

        LinearLayout linearLayout = new LinearLayout(ctx);
        linearLayout.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
        linearLayout.setOrientation(LinearLayout.VERTICAL);

        TabWidget tabWidget = new TabWidget(ctx);
        tabWidget.setId(android.R.id.tabs);

        FrameLayout frameLayout = new FrameLayout(ctx);
        frameLayout.setId(android.R.id.tabcontent);

        linearLayout.addView(tabWidget);
        linearLayout.addView(frameLayout);
        tabHost.addView(linearLayout);

        tabHost.setup();

        return tabHost;
    }

    @SuppressWarnings("deprecation")
    public static void addTab(View parent, String name, View child) {
        TabHost tabHost = (TabHost)parent;
        TabHost.TabSpec tab1Spec = tabHost.newTabSpec(name);
        tab1Spec.setIndicator(name);
        tab1Spec.setContent(new TabHost.TabContentFactory() {
            public View createTabContent(String tag) {
                return child;
            }
        });
        tabHost.addTab(tab1Spec);
    }

    public static boolean handleBack(boolean allowBack) {
        if (allowBack) {
            ((Activity)ctx).finish();
            return true;
        }
        return false;
    }

    public static boolean handleOptions(MenuItem item, boolean allowBack) {
        switch (item.getItemId()) {
            case android.R.id.home:
                handleBack(allowBack);
                return true;
        }

        return ((Activity)ctx).onOptionsItemSelected(item);
    }

    // Being too fast doesn't feel right, brain need delay
    public static void userSleep() {
        try {
            Thread.sleep(100);
        } catch (Exception e) {}
    }

    public static class Popup {
        PopupWindow popupWindow;
        public void dismiss() {
            this.popupWindow.dismiss();
        }

        String title;

        public void setChild(View v) {
            LinearLayout rel = new LinearLayout(ctx);

            LinearLayout bar = new LinearLayout(ctx);
            rel.setPadding(10, 10, 10, 10);
            rel.setOrientation(LinearLayout.HORIZONTAL);
            rel.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            TextView tv = new TextView(ctx);
            tv.setText(title);
            tv.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            tv.setPadding(20, 0, 0, 0);
            tv.setTextSize(20f);
            tv.setGravity(Gravity.CENTER);
            bar.addView(tv);

            rel.setOrientation(LinearLayout.VERTICAL);
            rel.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));

            rel.addView(bar);

            LinearLayout layout = new LinearLayout(ctx);
            layout.setPadding(20, 20, 20, 20);
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            layout.addView(v);
            rel.addView(layout);

            this.popupWindow.setContentView(rel);
            this.popupWindow.showAtLocation(((Activity)ctx).getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);
        }

        Popup(String title, int options) {
            userSleep();
            DisplayMetrics displayMetrics = new DisplayMetrics();
            ((Activity)ctx).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            int height = displayMetrics.heightPixels;
            int width = displayMetrics.widthPixels;
            this.title = title;

            this.popupWindow = new PopupWindow(
                    (int)(width / 1.2),
                    (int)(height / 1.9)
            );

            if (popupDrawableResource != 0) {
                this.popupWindow.setBackgroundDrawable(ctx.getResources().getDrawable(popupDrawableResource));
            }

            this.popupWindow.setOutsideTouchable(true);
        }
    }

    public static LibUI.Popup openWindow(String title, int options) {
        LibUI.Popup popup = new LibUI.Popup(title, options);
        return popup;
    }

    private static void runRunnable(byte[] data) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                LibUI.callFunction(data);
            }
        });
    }

    public static native void callFunction(byte[] struct);
    public static native void initThiz(Context ctx);
}
