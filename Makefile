PKG=dev.danielc.fujiapp
GRADLE=gradle
install:
	$(GRADLE) installDebug -Pandroid.optional.compilation=INSTANT_DEV -Pandroid.injected.build.api=24
	adb shell monkey -p $(PKG) -c android.intent.category.LAUNCHER 1

log:
	adb logcat | grep -F "`adb shell ps | grep $(PKG) | tr -s [:space:] ' ' | cut -d' ' -f2`"

rust:
	rustup target add armv7-linux-androideabi
	rustup target add i686-linux-android
	rustup target add aarch64-linux-android
	rustup target add x86_64-linux-android

# Update Daniel's dev hard symlinks
ln:
	rm -f app/src/main/java/camlib/*.java
	cd app/src/main/java/camlib/ && ln ../../../../../../camlibjava/*.java .

	rm -f app/src/main/java/libui/*.java
	cd app/src/main/java/libui/ && ln ../../../../../../libui-android/*.java .

	cd lib && ln ../../libui-android/*.c ../../libui-android/*.h .
