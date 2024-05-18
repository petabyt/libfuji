THEOS_DEVICE_USER=root
THEOS_PACKAGE_SCHEME=rootless
THEOS_DEVICE_IP=192.168.1.25


TARGET := iphone:clang:latest:7.0
INSTALL_TARGET_PROCESSES = fudgeios

include $(THEOS)/makefiles/common.mk

APPLICATION_NAME = fudgeios

$(APPLICATION_NAME)_FILES = main.m
$(APPLICATION_NAME)_FRAMEWORKS = UIKit CoreGraphics
$(APPLICATION_NAME)_CFLAGS = -fobjc-arc -Wno-unused-parameter -Wno-unused-variable

after-install::
	install.exec "open dev.danielc.fudge"

include $(THEOS_MAKE_PATH)/application.mk
