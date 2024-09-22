package dev.danielc.fujiapp;

import android.content.Context;
import android.content.Intent;

public class PtpThumbAdapter extends ThumbAdapter {
    private final int[] object_ids;
    public PtpThumbAdapter(Context ctx, int[] object_ids) {
        super(ctx);
        this.object_ids = object_ids;
        queue = new Queue();
    }

    Queue queue;
    class Queue extends DownloadQueue<PtpThumbAdapter.Request> {
        @Override
        void perform(Request req) {
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
        // TODO: Bring in ObjectIDs if idling
    }
    @Override
    void imageClickHandler(ImageViewHolder holder) {
        if (holder.isLoaded) {
            Intent intent = new Intent(holder.image.getContext(), Viewer.class);
            intent.putExtra("handle", holder.handle);
            holder.image.getContext().startActivity(intent);
        }
    }

    @Override
    void queueImage(ImageViewHolder holder, int position) {
        Request req = new Request();
        req.ctx = context;
        req.holder = holder;
        req.position = position;
        req.object_id = object_ids[position];
        holder.handle = req.object_id;
        queue.enqueue(req);
    }

    @Override
    void cancelRequest(ImageViewHolder holder) {
        for (int i = 0; i < queue.requests.size(); i++) {
            if (queue.requests.get(i).holder == holder) {
                queue.requests.remove(queue.requests.get(i));
            }
        }
    }

    @Override
    public int getItemCount() {
        return object_ids.length;
    }

    public static class Request {
        Context ctx;
        ImageViewHolder holder;
        int position;
        int object_id;
    };
}
