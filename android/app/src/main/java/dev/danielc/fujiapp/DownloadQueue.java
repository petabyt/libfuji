package dev.danielc.fujiapp;
import java.util.ArrayList;

public class DownloadQueue {
    private final ArrayList<Object> requests = new ArrayList<>();

    private volatile boolean stopDownloading = false;
    private volatile boolean pauseDownloading = false;

    void perform(Object request) {

    }

    public void enqueue(Object req) {
        synchronized (requests) {
            requests.add(req);
            requests.notifyAll();
        }
    }

    public synchronized void stopRequestThread() {
        stopDownloading = true;
        notifyAll();
    }

    public synchronized void pause() {
        pauseDownloading = true;
        notifyAll();
    }

    public synchronized void start() {
        stopDownloading = false;
        pauseDownloading = false;
        notifyAll();
    }

    void idle() {
        Object req = null;
        synchronized (requests) {
            while (requests.isEmpty()) {
                try {
                    requests.wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return;
                }
                if (stopDownloading) return;
            }
            req = requests.remove(requests.size() - 1);
        }
        if (req != null) {
            perform(req);
        }
    }

    public void startRequestThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (!stopDownloading) {
                    synchronized (DownloadQueue.this) {
                        while (pauseDownloading) {
                            try {
                                DownloadQueue.this.wait();
                            } catch (InterruptedException e) {
                                Thread.currentThread().interrupt();
                                return;
                            }
                        }
                    }
                    if (stopDownloading) return;
                    idle();
                }
            }
        }).start();
    }
}
