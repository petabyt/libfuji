package dev.danielc.views;
import java.util.ArrayList;

abstract public class DownloadQueue<T> extends Idler {
    public final ArrayList<T> requests = new ArrayList<>();

    public abstract void perform(T request);

    public void enqueue(T req) {
        synchronized (requests) {
            requests.add(req);
            requests.notifyAll();
        }
    }

    public boolean idle() {
        T req = null;
        synchronized (requests) {
            while (requests.isEmpty()) {
                try {
                    requests.wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return false;
                }
                if (stopDownloading) return false;
            }
            req = requests.remove(requests.size() - 1);
        }
        if (req != null) {
            perform(req);
        }
        return false;
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
