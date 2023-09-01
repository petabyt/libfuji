// All functions are single-threaded unless otherwise stated
package dev.danielc.fujiapp;

import org.json.JSONObject;

public class Camera {
	public static JSONObject openSession() throws Exception {
		return Backend.run("ptp_open_session");
	}

	public static JSONObject getObjectInfo(int handle) throws Exception {
		JSONObject jsonObject = Backend.run("ptp_get_object_info", new int[]{handle});
		return jsonObject;
	}
}
