package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.os.Bundle;
import android.view.MenuItem;

import dev.danielc.views.ThumbAdapter;

public class FileGallery extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle("Downloaded Photos");

        RecyclerView rv = new RecyclerView(this);
        rv.setLayoutManager(new GridLayoutManager(this, 4));
        rv.setNestedScrollingEnabled(false);
        ThumbAdapter imageAdapter = new FileThumbAdapter(this, Backend.getDownloads());
        rv.setAdapter(imageAdapter);
        rv.setItemViewCacheSize(50);
        rv.setNestedScrollingEnabled(false);
        setContentView(rv);
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}