package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import libui.LibUI;

public class Scripts extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle("Scripts");
        LibUI.start(this);
        LibUI.startWindow("fuji_scripts_screen");
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return LibUI.handleOptions(item, true);
    }
}