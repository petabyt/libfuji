package dev.danielc.fujiapp;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import org.json.JSONObject;
import org.w3c.dom.Text;

import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

public class PTPList extends BaseAdapter {
    int[] objectIDs;
    Queue queue;
    ListView lv;

    PTPList(int[] objectIDs, ListView lv) {
        this.lv = lv;
        this.objectIDs = objectIDs;
        this.queue = new Queue();
        for (int i = 0; i < objectIDs.length; i++) {
            Request req = new Request();
            req.position = i;
            req.object_id = objectIDs[i];
            req.list = this;
            queue.enqueue(req);
        }
        lv.setClickable(true);
        lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View v, int position, long id) {
                Intent intent = new Intent(v.getContext(), Viewer.class);
                intent.putExtra("handle", visible.get(position).object_id);
                v.getContext().startActivity(intent);
            }
        });
    }

    static class Holder {
        String filename;
        int size;
        int format;
        int object_id;
    }

    ArrayList<Holder> visible = new ArrayList<Holder>();

    @Override
    public int getCount() {
        return visible.size();
    }

    @Override
    public Object getItem(int i) {
        return visible.get(i);
    }

    @Override
    public long getItemId(int i) {
        return i;
    }

    public static class Request {
        PTPList list;
        int position;
        int object_id;
    };

    class Queue extends DownloadQueue {
        @Override
        void perform(Object request) {
            Request req = (Request)request;
            try {
                JSONObject info = Backend.getObjectInfo(req.object_id);
                Holder h = new Holder();
                h.object_id = req.object_id;
                h.filename = info.getString("filename");
                h.size = info.getInt("compressedSize");
                h.format = info.getInt("format_int");
                req.list.lv.post(new Runnable() {
                    @Override
                    public void run() {
                        req.list.visible.add(h);
                        req.list.notifyDataSetChanged();
                    }
                });
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public View getView(int i, View view, ViewGroup viewGroup) {
        LayoutInflater inflater = (LayoutInflater)viewGroup.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (view == null) {
            view = inflater.inflate(R.layout.item_file, viewGroup, false);
        }

        Holder h = visible.get(i);

        // TODO: performance
        TextView name = (TextView)view.findViewById(R.id.filename);
        TextView filesize = (TextView)view.findViewById(R.id.item_filesize);
        ImageView icon = (ImageView)view.findViewById(R.id.item_icon);
        name.setText(h.filename);
        filesize.setText(Frontend.formatFilesize(h.size));
        int format = h.format;
        if (format == Backend.PTP_OF_JPEG) {
            icon.setImageResource(R.drawable.baseline_landscape_24);
        } else {
            icon.setImageResource(R.drawable.baseline_movie_24);
        }

        return view;
    }
}