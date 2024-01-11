use std::ffi::CString;
use std::ffi::CStr;
use std::os::raw::c_char;

mod c {
	extern {
		pub fn app_print(fmt: *const u8, ...);
		pub fn android_err(fmt: *const u8, ...);
	}
}

fn app_print(string: &str) {
	let str = CString::new(string).unwrap().into_raw() as *const u8;
	unsafe {
		c::app_print(str);
	}
}

#[no_mangle]
pub fn fudgers_test(data_ptr: *const u8) -> u32 {
	app_print("Hello, Rustardsasd");
	return 0;
}
