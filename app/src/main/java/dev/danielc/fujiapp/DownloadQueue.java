// This really sucks (for now)
package dev.danielc.fujiapp;

import android.view.View;

import java.util.ArrayList;

abstract public class DownloadQueue {
    // LIFO stack
    ArrayList<Object> requests = new ArrayList<Object>();

    abstract void perform(Object request);

    void enqueue(Object req) {
        requests.add(req);
    }

    volatile boolean haveCompletelyStopped = false;
    boolean stop_downloading = false;
    boolean pause_downloading = false;

    void stopRequestThread() {
        stop_downloading = true;
//        while (!haveCompletelyStopped) {
            // ...
//        }
    }
    void pause() {
        pause_downloading = true;
    }
    void start() {
        pause_downloading = false;
    }

    void startRequestThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (!stop_downloading) {
                    if (requests.size() == 0 || pause_downloading) {
                        try {
                            Thread.sleep(100);
                        } catch (Exception ignored) {}
                        continue;
                    }
                    Object req = requests.remove(requests.size() - 1);
                    perform(req);
                }
                haveCompletelyStopped = true;
            }
        }).start();
    }
}
