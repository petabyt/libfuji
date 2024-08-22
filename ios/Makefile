THEOS_DEVICE_USER=root
THEOS_PACKAGE_SCHEME=rootless
THEOS_DEVICE_IP=192.168.1.25
PACKAGE_FORMAT=ipa

TARGET := iphone:clang:latest:7.0
INSTALL_TARGET_PROCESSES = fudgeios

include $(THEOS)/makefiles/common.mk

APPLICATION_NAME = fudgeios

CAMLIB := ../camlib
CAMLIB_CORE := $(addprefix $(CAMLIB)/src/,transport.c data.c enum_dump.c enums.c canon.c operations.c packet.c lib.c conv.c generic.c)

FUDGE_CORE := $(addprefix ../lib/,net.c tester.c fuji.c fuji_usb.c viewer.c data.c)

$(APPLICATION_NAME)_FILES = main.m lib.m layout.m $(CAMLIB_CORE) $(FUDGE_CORE)
$(APPLICATION_NAME)_FRAMEWORKS = UIKit CoreGraphics
$(APPLICATION_NAME)_CFLAGS = -fobjc-arc -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -I$(CAMLIB)/src -I../lib/
$(APPLICATION_NAME)_PACKAGE_FORMAT = ipa
_THEOS_TARGET_DEFAULT_PACKAGE_FORMAT = ipa

after-install::
	install.exec "open dev.danielc.fudge"

include $(THEOS_MAKE_PATH)/application.mk
