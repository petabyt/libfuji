package dev.danielc.fujiapp;

import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class ObjectInfoAdapter extends BaseAdapter {
    int[] objectIDs;
    Queue queue;
    ListView lv;
    JSONObject[] list;

    ObjectInfoAdapter(int[] objectIDs, ListView lv) {
        this.list = new JSONObject[0];
        this.lv = lv;
        this.objectIDs = objectIDs;
        this.queue = new Queue();
        lv.setClickable(true);
        lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View v, int position, long id) {
                Intent intent = new Intent(v.getContext(), Viewer.class);
                intent.putExtra("handle", objectIDs[position]);
                v.getContext().startActivity(intent);
            }
        });
    }

    @Override
    public int getCount() {
        return list.length;
    }

    @Override
    public Object getItem(int i) {
        return list[i];
    }

    @Override
    public long getItemId(int i) {
        return i;
    }

    public void updateList(JSONObject[] handles) {
        list = Backend.cPtpObjectServiceGetFilled();
        lv.post(new Runnable() {
            @Override
            public void run() {
                notifyDataSetChanged();
            }
        });
    }

    static class Queue extends DownloadQueue {
        @Override
        void idle() {
            int rc = Backend.cPtpObjectServiceStep();
            if (rc != 0) {
                stopRequestThread();
                Backend.reportError(Backend.PTP_IO_ERR, "Failed to download image");
            }
        }
    }

    @Override
    public View getView(int i, View view, ViewGroup viewGroup) {
        LayoutInflater inflater = (LayoutInflater)viewGroup.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (view == null) {
            view = inflater.inflate(R.layout.item_file, viewGroup, false);
        }

        JSONObject info = list[i];
        try {
            String filename = info.getString("filename");
            int size = info.getInt("compressedSize");
            int format = info.getInt("format_int");
            // TODO: optimize
            TextView name = (TextView)view.findViewById(R.id.filename);
            TextView filesize = (TextView)view.findViewById(R.id.item_filesize);
            ImageView icon = (ImageView)view.findViewById(R.id.item_icon);
            name.setText(filename);
            filesize.setText(Frontend.formatFilesize(size));
            if (format == Backend.PTP_OF_JPEG) {
                icon.setImageResource(R.drawable.baseline_landscape_24);
            } else {
                icon.setImageResource(R.drawable.baseline_movie_24);
            }
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }

        return view;
    }
}