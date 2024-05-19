package dev.danielc.fujiapp;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class Liveview extends AppCompatActivity implements SurfaceHolder.Callback {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_liveview);
        //Log.d("liveview", "crated");

        SurfaceView x = new SurfaceView(this);
        x.getHolder().addCallback(this);
        setContentView(x);
    }

    public static native void cUpdateSurface(Surface holder);

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
//        Canvas canvas = surfaceHolder.lockCanvas();
//        Paint p = new Paint();
//        p.setColor(Color.RED);
//        canvas.drawColor(Color.BLACK);
//        canvas.drawRect(0, 0, 100, 100, p);
//        surfaceHolder.unlockCanvasAndPost(canvas);

        cUpdateSurface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }
}