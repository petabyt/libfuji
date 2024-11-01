package dev.danielc.views;

abstract public class Idler {
    public volatile boolean stopDownloading = false;
    public volatile boolean pauseDownloading = false;

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

    /** Return false if action was done, true if no action was done */
    public abstract boolean idle();

    public void startRequestThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (!stopDownloading) {
                    synchronized (Idler.this) {
                        while (pauseDownloading) {
                            try {
                                Idler.this.wait();
                            } catch (InterruptedException e) {
                                Thread.currentThread().interrupt();
                                return;
                            }
                        }
                    }
                    if (stopDownloading) return;
                    if (idle()) {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException ignored) {}
                    }
                }
            }
        }).start();
    }
}
