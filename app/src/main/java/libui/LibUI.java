package libui;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Lifecycle;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;

import java.util.ArrayList;

public class LibUI {
    public static Context ctx = null;
    static Menu menu = null;

    // uiWindow (popup) background drawable style resource
    public static int popupDrawableResource = 0;

    // Background drawable resource for buttons
    public static int buttonBackgroundResource = 0;

    public static void init(Activity act) {
        ctx = (Context)act;
    }

    public static void start(Activity act) {
        init(act);
        initThiz(ctx);
    }

    // Common way of telling when activity is done loading
    public static void waitUntilActivityLoaded(Activity activity) {
        ViewTreeObserver viewTreeObserver = activity.getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                activity.getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
            }
        });
    }

    public static class MyFragment extends Fragment {
        ViewGroup view;
        MyFragment(ViewGroup v) {
            view = v;
        }
        @Nullable
        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            return view;
        }
    }

    public static class MyFragmentStateAdapter extends FragmentStateAdapter {
        private ArrayList<ViewGroup> arrayList = new ArrayList<>();
        public MyFragmentStateAdapter(@NonNull FragmentManager fragmentManager, @NonNull Lifecycle lifecycle) {
            super(fragmentManager, lifecycle);
        }

        public void addViewGroup(ViewGroup vg) {
            arrayList.add(vg);
        }

        @NonNull
        @Override
        public Fragment createFragment(int position) {
            return new MyFragment(arrayList.get(position));
        }

        @Override
        public int getItemCount() {
            return arrayList.size();
        }
    }

    public static class MyOnClickListener implements View.OnClickListener {
        private long ptr;
        private long arg1;
        private long arg2;
        public MyOnClickListener(long ptr, long arg1, long arg2) {
            this.ptr = ptr;
            this.arg1 = arg1;
            this.arg2 = arg2;
        }

        @Override
        public void onClick(View v) {
            LibUI.callFunction(ptr, arg1, arg2);
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

    public static View tabLayout() {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT,
                1.0f
        ));

        TabLayout tl = new TabLayout(ctx);
        tl.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        ViewPager2 pager = new ViewPager2(ctx);
        pager.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        MyFragmentStateAdapter frag = new MyFragmentStateAdapter(
                ((AppCompatActivity)ctx).getSupportFragmentManager(),
                ((AppCompatActivity)ctx).getLifecycle()
        );
        pager.setAdapter(frag);

        tl.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                pager.setCurrentItem(tab.getPosition());
            }
            @Override
            public void onTabUnselected(TabLayout.Tab tab) {}
            @Override
            public void onTabReselected(TabLayout.Tab tab) {}
        });

        pager.registerOnPageChangeCallback(new ViewPager2.OnPageChangeCallback() {
            @Override
            public void onPageSelected(int position) {
                tl.selectTab(tl.getTabAt(position));
            }
        });

        layout.addView(tl);
        layout.addView(pager);

        return layout;
    }

    private static void addTab(View parent, String name, View child) {
        // TabLayout is child 0, add a new tab
        TabLayout tl = (TabLayout)(((ViewGroup)parent).getChildAt(0));
        TabLayout.Tab tab = tl.newTab();
        tab.setText(name);
        tl.addTab(tab);

        // ViewPager2 is the second child, we can get the custom fragment adapter from it
        ViewPager2 vp = (ViewPager2)(((ViewGroup)parent).getChildAt(1));
        MyFragmentStateAdapter frag = (MyFragmentStateAdapter)vp.getAdapter();

        ScrollView sv = new ScrollView(ctx);
        sv.addView(child);

        frag.addViewGroup(sv);
    }

    public static class Screen {
        int displayOptions;
        int id;
        String title;
        View content;
    };

    static ArrayList<Screen> screens = new ArrayList<Screen>();
    static Screen origActivity = new Screen();

    public static void switchScreen(View view, String title) {
        userSleep();

        ActionBar actionBar = ((AppCompatActivity)ctx).getSupportActionBar();

        ScrollView layout = new ScrollView(ctx);
        layout.addView(view);

        Screen screen = new Screen();
        screen.id = screens.size();
        screen.title = title;
        screen.content = layout;

        if (screens.size() == 0) {
            origActivity.content = ((ViewGroup)((Activity)ctx).findViewById(android.R.id.content)).getChildAt(0);
            origActivity.title = (String)actionBar.getTitle();
            origActivity.displayOptions = actionBar.getDisplayOptions();
        }

        screens.add(screen);

        // Now start constructing the new layout
        ((Activity)ctx).setContentView(layout);
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(title);
        if (menu != null) {
            for (int i = 0; i < menu.size(); i++) {
                menu.getItem(i).setVisible(false);
            }
        }
    }

    private static void screenGoBack() {
        Screen screen = screens.remove(screens.size() - 1);

        if (screens.size() == 0) {
            ((Activity)ctx).setContentView(origActivity.content);

            ActionBar actionBar = ((AppCompatActivity)ctx).getSupportActionBar();
            actionBar.setTitle(origActivity.title);
            if ((origActivity.displayOptions & ActionBar.DISPLAY_SHOW_HOME) != 0) {
                actionBar.setDisplayHomeAsUpEnabled(true);
            } else {
                actionBar.setDisplayHomeAsUpEnabled(false);
            }
            if (menu != null) {
                for (int i = 0; i < menu.size(); i++) {
                    menu.getItem(i).setVisible(true);
                }
            }
        } else {
            ((Activity)ctx).setContentView(screen.content);

            ActionBar actionBar = ((AppCompatActivity)ctx).getSupportActionBar();
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(screen.title);
        }
    }

    public static boolean handleBack(boolean allowBack) {
        if (screens.size() == 0) {
            if (allowBack) {
                ((Activity)ctx).finish();
                return true;
            }
        } else {
            screenGoBack();
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

    public static boolean handleMenu(Menu m) {
        menu = m;
        Log.d("libui", "menu added");
        return true;
    }

    // TODO: Use Handler.postDelayed?
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
            rel.setPadding(30, 10, 10, 10);
            rel.setOrientation(LinearLayout.HORIZONTAL);
            rel.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            TextView tv = new TextView(ctx);
            tv.setText(title);
            tv.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            tv.setTextSize(20f);
            bar.addView(tv);

            rel.setOrientation(LinearLayout.VERTICAL);
            rel.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));

            rel.addView(bar);

            LinearLayout layout = new LinearLayout(ctx);
            layout.setPadding(0, 20, 20, 20);
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
                this.popupWindow.setBackgroundDrawable(ContextCompat.getDrawable(ctx, popupDrawableResource));
            }

            this.popupWindow.setOutsideTouchable(true);
        }
    }

    private static LibUI.Popup openWindow(String title, int options) {
        LibUI.Popup popup = new LibUI.Popup(title, options);
        return popup;
    }

    private static void setClickListener(View v, long ptr, long arg1, long arg2) {
        v.setOnClickListener(new MyOnClickListener(ptr, arg1, arg2));
    }

    private static ViewGroup linearLayout(int orientation) {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(orientation);
        layout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        return (ViewGroup)layout;
    }

    private static void setPadding(View v, int l, int t, int r, int b) {
        v.setPadding(l, t, r, b);
    }

    private static String getString(String name) {
        Resources res = ctx.getResources();
        return res.getString(res.getIdentifier(name, "string", ctx.getPackageName()));
    }

    private static View getView(String name) {
        Resources res = ctx.getResources();
        int id = res.getIdentifier(name, "id", ctx.getPackageName());
        return ((Activity)ctx).findViewById(id);
    }

    private static void toast(String text) {
        Toast.makeText(ctx, text, Toast.LENGTH_SHORT).show();
    }

    private static void runRunnable(long ptr, long arg1, long arg2) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                callFunction(ptr, arg1, arg2);
            }
        });
    }

    private static native void callFunction(long ptr, long arg1, long arg2);
    public static native void initThiz(Context ctx);
    public static native void startWindow(String symbolName);
    public static native void startNative(String symbolName);

//    private int dpToPx(int dp) {
//        Resources r = ctx.getResources();
//        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, r.getDisplayMetrics()));
//    }
}
