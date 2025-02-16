package dev.danielc.fujiapp;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashSet;

import dev.danielc.views.DownloadQueue;
import dev.danielc.views.ThumbAdapter;

public class PtpThumbAdapter extends ThumbAdapter {
    public PtpThumbAdapter(Context ctx) {
        super(ctx);
        queue = new Queue();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void sort() {
        // TODO:
        notifyDataSetChanged();
    }

    public void setupHolderFromInfo(ImageViewHolder holder, JSONObject info) {
        holder.image.post(new Runnable() {
            @Override
            public void run() {
                try {
                    holder.label.setText(info.getString("filename"));
                    int format = info.getInt("format_int");

                    if (format == Backend.PTP_OF_JPEG) {
                        holder.icon.setImageResource(R.drawable.baseline_landscape_24);
                    } else if (format == Backend.PTP_OF_MOV) {
                        holder.icon.setImageResource(R.drawable.baseline_movie_24);
                    } else if (format == Backend.PTP_OF_RAW) {
                        holder.icon.setImageResource(R.drawable.baseline_data_array_24);
                    } else {
                        holder.icon.setImageResource(R.drawable.baseline_question_mark_24);
                    }

                } catch (JSONException e) {
                    throw new RuntimeException(e);
                }
            }
        });
    }

    public void updateInfoOnHolder(int handle, JSONObject info) {
        for (int i = 0; i < holders.size(); i++) {
            if (holders.get(i).handle == handle) {
                setupHolderFromInfo(holders.get(i), info);
            }
        }
    }

    public static class Request {
        Context ctx;
        ImageViewHolder holder;
        int position;
        int object_id;
    }

    Queue queue;
    class Queue extends DownloadQueue<Request> {
        @Override
        public void perform(Request req) {
            if (Backend.cGetKillSwitch()) return;
            byte[] jpegByteArray = Backend.cFujiGetThumb(req.object_id);
            if (jpegByteArray == null) {
                Backend.reportError(Backend.PTP_IO_ERR, "Failed to get thumbnail");
                return;
            } else if (jpegByteArray.length == 0) {
                // Unable to find thumbnail - assume it's a folder or non-jpeg
                invalidThumb(req.holder.image.getContext(), req.holder);
                return;
            }
            loadThumb(req.holder, jpegByteArray);
        }
        @Override
        public boolean nothingToDo() {
            int rc = Backend.cPtpObjectServiceStep();
            if (rc < 0) {
                Backend.reportError(Backend.PTP_IO_ERR, "Failed to get object info");
                stopRequestThread();
            }
            if (rc > 0) {
                updateInfoOnHolder(rc, Backend.cPtpObjectServiceGet(rc));
                return false;
            }
            return true;
        }
    }
    @Override
    public void imageClickHandler(ImageViewHolder holder) {
        if (holder.isLoaded) {
            Intent intent = new Intent(holder.image.getContext(), Viewer.class);
            intent.putExtra("handle", holder.handle);
            holder.image.getContext().startActivity(intent);
        }
    }

    @Override
    public void queueImage(ImageViewHolder holder, int position) {
        Request req = new Request();
        req.ctx = context;
        req.holder = holder;
        req.position = position;
        req.object_id = Backend.cObjectServiceGetHandleAt(position);
        holder.handle = req.object_id;
        queue.enqueue(req);

        long start = System.nanoTime();

        JSONObject info = Backend.cPtpObjectServiceGet(holder.handle);
        if (info != null) {
            setupHolderFromInfo(holder, info);
        }
    }

    @Override
    public void cancelRequest(ImageViewHolder holder) {
        for (int i = 0; i < queue.requests.size(); i++) {
            if (queue.requests.get(i).holder == holder) {
                queue.requests.remove(queue.requests.get(i));
            }
        }
    }

    @Override
    public int getItemCount() {
        return Backend.cObjectServiceLength();
    }
}
