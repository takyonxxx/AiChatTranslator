TEMPLATE = app
QT += core gui multimedia network texttospeech widgets
TARGET = SpeechTranslator
CONFIG += c++17

# Android-specific configurations
android {
    message("Android enabled")
    QT += core-private

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    ANDROID_PLATFORM = android-34
    ANDROID_MIN_SDK_VERSION = 28
    ANDROID_ABIS = arm64-v8a
    ANDROID_VERSION_CODE = 1
    ANDROID_VERSION_NAME = "1.0"

    # OpenSSL configuration
    ANDROID_OPENSSL_ROOT = C:\Users\MarenCompEng\Desktop\lib\openssl\ssl_3

    INCLUDEPATH += $$ANDROID_OPENSSL_ROOT/include

# Specify the full names of the OpenSSL libraries
   ANDROID_EXTRA_LIBS += \
       $$ANDROID_OPENSSL_ROOT/armeabi-v7a/libcrypto_3.so \
       $$ANDROID_OPENSSL_ROOT/armeabi-v7a/libssl_3.so

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

# macOS-specific configurations
macx {
    message("macOS enabled")
    RC_ICONS = $$PWD/icons/ai.ico
    QMAKE_INFO_PLIST = $$PWD/Info.plist.in
    LIBS += -framework AudioToolbox
}

# iOS-specific configurations
ios {
    message("iOS enabled")
    QMAKE_INFO_PLIST = ios/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"
    LIBS += -framework AVFoundation
    LIBS += -framework AudioToolbox
}

# Windows-specific configurations
win32 {
    message("Windows enabled")
    INCLUDEPATH += $$PWD
}

# Linux-specific configurations
unix:!mac:!android {
    message("Linux enabled")
}

# Header files
HEADERS = \
    audiolevel.h \
    constants.h \
    gpt3client.h \
    mainwindow.h \
    translateclient.h

# Source files
SOURCES = \
    main.cpp \
    audiolevel.cpp \
    mainwindow.cpp

# Forms
FORMS += \
    mainwindow.ui

# Resources
RESOURCES += \
    resources.qrc

# Include shared project file
include(./shared/shared.pri)

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/drawable-xhdpi/icon.png \
    android/res/drawable-xxhdpi/icon.png \
    android/res/drawable-xxxhdpi/icon.png \
    android/res/values/libs.xml \
    android/res/xml/qtprovider_paths.xml
