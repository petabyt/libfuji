package dev.danielc.fujiapp;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.GridLayoutManager;
import java.util.List;
import java.util.ArrayList;


public class test extends AppCompatActivity {

    private RecyclerView recyclerView;
    private ImageAdapter imageAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, 4));

        int[] ids = {1, 2, 3};

        imageAdapter = new ImageAdapter(this, ids);
        recyclerView.setAdapter(imageAdapter);
    }
}



