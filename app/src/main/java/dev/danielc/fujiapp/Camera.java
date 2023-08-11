package dev.danielc.fujiapp;

public class Camera {
	public static void openSession() throws Exception {
		Backend.run("ptp_open_session");
	}

	public static void getObjectInfo(int handle) throws Exception {
		Backend.run("ptp_get_object_info", new int[]{handle});
	}
}
