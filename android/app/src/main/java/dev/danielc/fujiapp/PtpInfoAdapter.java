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

import dev.danielc.common.Camlib;
import dev.danielc.views.Idler;

public class ObjectInfoAdapter extends BaseAdapter {
    int[] objectIDs;
    Queue queue;
    ListView lv;
    JSONObject[] list;
    static int lastSelected = 0;

    ObjectInfoAdapter(int[] objectIDs, ListView lv) {
        this.list = Backend.cPtpObjectServiceGetFilled();
        this.lv = lv;
        this.objectIDs = objectIDs;
        this.queue = new Queue();
        lv.setClickable(true);
        lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View v, int position, long id) {
                Intent intent = new Intent(v.getContext(), Viewer.class);
                lastSelected = position;
                notifyDataSetInvalidated();
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

    class Queue extends Idler {
        @Override
        public boolean idle() {
            int rc = Backend.cPtpObjectServiceStep();
            if (rc < 0) {
                stopRequestThread();
                Backend.reportError(Backend.PTP_IO_ERR, "Failed to get object info");
            }
            if (rc > 0) {
                JSONObject[] newList = Backend.cPtpObjectServiceGetFilled();
                lv.post(new Runnable() {
                    @Override
                    public void run() {
                        list = newList;
                        notifyDataSetChanged();
                    }
                });
                return false;
            }
            return true;
        }
    }

    @Override
    public View getView(int i, View view, ViewGroup viewGroup) {
        LayoutInflater inflater = (LayoutInflater)viewGroup.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (view == null) {
            view = inflater.inflate(R.layout.item_file, viewGroup, false);
        }

        String filename;
        int size;
        int format;

        JSONObject info = list[i];
        if (info == null) {
            filename = "?";
            size = 0;
            format = Camlib.PTP_OF_Undefined;
        } else {
            try {
                filename = info.getString("filename");
                size = info.getInt("compressedSize");
                format = info.getInt("format_int");
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }
        }

        if (i == lastSelected && i != 0) {
            view.setBackgroundResource(R.drawable.selected);
        } else {
            view.setBackgroundDrawable(null);
        }

        TextView name = (TextView)view.findViewById(R.id.filename);
        TextView filesize = (TextView)view.findViewById(R.id.item_filesize);
        ImageView icon = (ImageView)view.findViewById(R.id.item_icon);
        name.setText(filename);
        filesize.setText(Frontend.formatFilesize(size));
        if (format == Backend.PTP_OF_JPEG) {
            icon.setImageResource(R.drawable.baseline_landscape_24);
        } else if (format == Backend.PTP_OF_MOV) {
            icon.setImageResource(R.drawable.baseline_movie_24);
        } else if (format == Backend.PTP_OF_RAW) {
            icon.setImageResource(R.drawable.baseline_data_array_24);
        } else {
            icon.setImageResource(R.drawable.baseline_question_mark_24);
        }

        return view;
    }
}