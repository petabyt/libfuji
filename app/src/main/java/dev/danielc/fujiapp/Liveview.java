package dev.danielc.fujiapp;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;

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

    public static byte[] renderText(String text, int fg, int bg) {
        Bitmap bitmap = Bitmap.createBitmap(256, 256, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        bitmap.eraseColor(0x0);
        Paint textPaint = new Paint();
        textPaint.setTextSize(32);
        textPaint.setAntiAlias(true);
        textPaint.setColor(0xFFFFFFFF);
        canvas.drawText("Hello World", 10, 40, textPaint);

        ByteBuffer byteBuffer = ByteBuffer.allocate(bitmap.getByteCount());
        bitmap.copyPixelsToBuffer(byteBuffer);
        byteBuffer.rewind();
        return byteBuffer.array();
    }

    public static native void cUpdateSurface(Surface holder);

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
        cUpdateSurface(surfaceHolder.getSurface());

        Canvas canvas = surfaceHolder.lockCanvas(new Rect(0, 0, 100, 100));
        Paint p = new Paint();
        p.setColor(Color.RED);
        //canvas.drawColor(Color.BLACK);
        canvas.drawRect(0, 0, 100, 100, p);
        surfaceHolder.unlockCanvasAndPost(canvas);
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }
}