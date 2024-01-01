PKG=dev.danielc.fujiapp

install:
	#bash gradlew :app:buildCMakeDebug[arm64-v8a] installDebug -Pandroid.optional.compilation=INSTANT_DEV -Pandroid.injected.build.api=24
	bash gradlew installDebug -Pandroid.optional.compilation=INSTANT_DEV -Pandroid.injected.build.api=24
	adb shell monkey -p $(PKG) -c android.intent.category.LAUNCHER 1

log:
	adb logcat | grep -F "`adb shell ps | grep $(PKG) | tr -s [:space:] ' ' | cut -d' ' -f2`"

ln:
	rm -f app/src/main/java/camlib/*.java
	cd app/src/main/java/camlib/ && ln ../../../../../../camlibjava/*.java .

	rm -f app/src/main/java/libui/*.java
	cd app/src/main/java/libui/ && ln ../../../../../../libui-android/*.java .

	cd lib && ln ../../libui-android/*.c ../../libui-android/*.h .
