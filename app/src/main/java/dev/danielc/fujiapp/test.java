package dev.danielc.fujiapp;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.GridLayoutManager;
import java.util.List;
import java.util.ArrayList;
import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupWindow;
import android.os.Handler;
import android.os.Looper;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.Button;
import android.widget.ProgressBar;


public class test extends AppCompatActivity {
    private Handler handler;
    public void showPopupWindow(Activity activity) {
        // Create a LayoutInflater object to inflate the popup layout
        LayoutInflater inflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.popup_download, null);

        // Create a PopupWindow object
        PopupWindow popupWindow = new PopupWindow(popupView, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        // Set a background color for the popup window
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        // Set the window position
        popupWindow.showAtLocation(getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);

        // Dismiss the popup window when clicked outside or the back button is pressed
        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(true);

        ProgressBar progressBar = popupView.findViewById(R.id.progress_bar);
        progressBar.setProgress(50); // Set the progress to 50%

        // Handle the click event of the close button
        Button closeButton = popupView.findViewById(R.id.close_button);
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                popupWindow.dismiss();
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);

        // Wait until activity token is ready
        ViewTreeObserver viewTreeObserver = getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                // Remove the listener to prevent multiple calls
                getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);

                // Show the popup window
                showPopupWindow(test.this);
            }
        });
    }
}



