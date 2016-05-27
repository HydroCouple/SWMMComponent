#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2016
#License GNU General Public License (see <http://www.gnu.org/licenses/> for details).
#EPA SWMM Model Component

TEMPLATE = lib
TARGET = SWMMComponent
QT -= core


DEFINES += SWMMCOMPONENT_LIBRARY

INCLUDEPATH += .\
               ./include \
               ./../HydroCouple/include \
               ./../HydroCoupleSDK/include
       
PRECOMPILED_HEADER = ./include/stdafx.h

HEADERS += ./include/consts.h \
           ./include/datetime.h \
           ./include/enums.h \
           ./include/error.h \
           ./include/exfil.h \
           ./include/findroot.h \
           ./include/funcs.h \
           ./include/globals.h \
           ./include/hash.h \
           ./include/headers.h \
           ./include/infil.h \
           ./include/keywords.h \
           ./include/lid.h \
           ./include/macros.h \
           ./include/mathexpr.h \
           ./include/mempool.h \
           ./include/objects.h \
           ./include/odesolve.h \
           ./include/stdafx.h \
           ./include/swmm5.h \
           ./include/swmmcomponent.h \
           ./include/swmmcomponent_global.h \
           ./include/text.h \
           ./include/swmmmodelcomponentinfo.h \
           ./include/dataexchangecache.h \
           ./include/swmmobjectitems.h \
           ./include/swmmtimeseriesexchangeitems.h
          
SOURCES +=./src/climate.c \
          ./src/controls.c \
          ./src/culvert.c \
          ./src/datetime.c \
          ./src/dwflow.c \
          ./src/dynwave.c \
          ./src/error.c \
          ./src/exfil.c \
          ./src/findroot.c \
          ./src/flowrout.c \
          ./src/forcmain.c \
          ./src/gage.c \
          ./src/gwater.c \
          ./src/hash.c \
          ./src/hotstart.c \
          ./src/iface.c \
          ./src/infil.c \
          ./src/inflow.c \
          ./src/input.c \
          ./src/inputrpt.c \
          ./src/keywords.c \
          ./src/kinwave.c \
          ./src/landuse.c \
          ./src/lid.c \
          ./src/lidproc.c \
          ./src/link.c \
          ./src/massbal.c \
          ./src/mathexpr.c \
          ./src/mempool.c \
          ./src/node.c \
          ./src/odesolve.c \
          ./src/output.c \
          ./src/project.c \
          ./src/qualrout.c \
          ./src/rain.c \
          ./src/rdii.c \
          ./src/report.c \
          ./src/roadway.c \
          ./src/routing.c \
          ./src/runoff.c \
          ./src/shape.c \
          ./src/snow.c \
          ./src/stats.c \
          ./src/statsrpt.c \
          ./src/stdafx.cpp \
          ./src/subcatch.c \
          ./src/surfqual.c \
          ./src/swmm5.c \
          ./src/swmmcomponent.cpp \
          ./src/table.c \
          ./src/toposort.c \
          ./src/transect.c \
          ./src/treatmnt.c \
          ./src/xsect.c \
          ./src/dataexchangecache.cpp \
          ./src/swmmmodelcomponentinfo.cpp \
          ./src/swmmtimeseriesexchangeitems.cpp


macx{

    INCLUDEPATH += /usr/local/include \
                   /usr/local/include/libiomp

#    QMAKE_CC = clang-omp
#    QMAKE_CXX = clang-omp++
#    QMAKE_LINK = $$QMAKE_CXX

#    QMAKE_CFLAGS = -fopenmp
#    QMAKE_LFLAGS = -fopenmp
#    QMAKE_CXXFLAGS = -fopenmp
#    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS
#    QMAKE_CXXFLAGS_DEBUG = $$QMAKE_CXXFLAGS

    LIBS += -L/usr/local/lib/ -liomp5
 }

CONFIG(debug, debug|release) {

   DESTDIR = ./build/debug
   OBJECTS_DIR = $$DESTDIR/.obj
   MOC_DIR = $$DESTDIR/.moc
   RCC_DIR = $$DESTDIR/.qrc
   UI_DIR = $$DESTDIR/.ui

   macx{
    LIBS += -L./../HydroCoupleSDK/build/debug -lHydroCoupleSDK.1.0.0
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
    LIBS += -L./../HydroCoupleSDK/lib -lHydroCoupleSDK.1.0.0
   }
}   
