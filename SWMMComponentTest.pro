#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2016
#License GNU General Public License (see <http://www.gnu.org/licenses/> for details).
#EPA SWMM Model Component

TEMPLATE = app
TARGET = SWMMComponentTest
QT       += core testlib

CONFIG   -= console
CONFIG   -= app_bundle

INCLUDEPATH += .\
               ./include \
               ./include/test \
               ./../HydroCouple/include \
               ./../HydroCoupleSDK/include

PRECOMPILED_HEADER = ./include/stdafx.h

HEADERS += ./include/stdafx.h \
    include/test/swmmcomponenttest.h

SOURCES += \
    src/test/main.cpp

CONFIG(debug, debug|release) {

   DESTDIR = ./build/debug
   OBJECTS_DIR = $$DESTDIR/.obj
   MOC_DIR = $$DESTDIR/.moc
   RCC_DIR = $$DESTDIR/.qrc
   UI_DIR = $$DESTDIR/.ui

   macx{
    LIBS += -L./../HydroCoupleSDK/build/debug -lHydroCoupleSDK.1.0.0 \
            -L./build/debug -lSWMMComponent.1.0.0
   }
}

CONFIG(release, debug|release) {

    DESTDIR = lib
    RELEASE_EXTRAS = ./build/release
    OBJECTS_DIR = $$RELEASE_EXTRAS/.obj
    MOC_DIR = $$RELEASE_EXTRAS/.moc
    RCC_DIR = $$RELEASE_EXTRAS/.qrc
    UI_DIR = $$RELEASE_EXTRAS/.ui

   macx{
    LIBS += -L./../HydroCoupleSDK/lib -lHydroCoupleSDK.1.0.0 \
            -L./lib -lSWMMComponent.1.0.0
   }
}



