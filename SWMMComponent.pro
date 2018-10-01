#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2016 - 2018
#License GNU Lesser General Public License (see <http: //www.gnu.org/licenses/> for details).
#EPA SWMM Model Component

TEMPLATE = lib
VERSION = 1.0.0
TARGET = SWMMComponent
QT -= gui

DEFINES += SWMMCOMPONENT_LIBRARY
DEFINES += USE_OPENMP
DEFINES += USE_MPI
SWMM_VERSION = 5.1.012
#DEFINES += QT_NO_VERSION_TAGGING

CONFIG += c++11
CONFIG += debug_and_release
CONFIG += optimize_full


contains(DEFINES,SWMMCOMPONENT_LIBRARY){
  TEMPLATE = lib
  message("Compiling as library")
} else {
  TEMPLATE = app
  CONFIG-=app_bundle
  message("Compiling as application")
}

win32{
  INCLUDEPATH += $$PWD/graphviz/win32/include \
                 $$(MSMPI_INC)/
}

PRECOMPILED_HEADER = ./include/stdafx.h

INCLUDEPATH += .\
               ./include \
               ./../SWMM/include \
               ./../SWMM/$$SWMM_VERSION/include \
               ./../HydroCouple/include \
               ./../HydroCoupleSDK/include


HEADERS += ./include/stdafx.h\
           ./include/swmmcomponent_global.h \
           ./include/swmmcomponent.h \
           ./include/swmmcomponentinfo.h \
           ./include/swmmobjectitems.h \
           ./include/swmmtimeseriesexchangeitems.h \
           ./include/pondedareainput.h \
           ./include/nodesurfaceflowoutput.h \
           ./include/nodewseinput.h \
           ./include/nodepondeddepthinput.h \
           ./include/linkflowoutput.h \
           ./include/linkdepthoutput.h \
           ./include/conduitxsectareaoutput.h \
           ./include/conduitbankxsectareaoutput.h \
           ./include/conduittopwidthoutput.h

SOURCES +=./src/stdafx.cpp \
          ./src/swmmcomponent.cpp \
          ./src/swmmcomponentinfo.cpp \
          ./src/swmmtimeseriesexchangeitems.cpp \
          ./src/pondedareainput.cpp \
          ./src/nodesurfaceflowoutput.cpp \
          ./src/nodewseinput.cpp \
          ./src/nodepondeddepthinput.cpp \
          ./src/linkflowoutput.cpp \
          ./src/main.cpp \
          ./src/linkdepthoutput.cpp \
          ./src/conduitxsectareaoutput.cpp \
          ./src/conduittopwidthoutput.cpp \
          ./src/conduitbankxsectareaoutput.cpp


macx{

    INCLUDEPATH += /usr/local \
                   /usr/local/include \
                   /usr/local/include/gdal

    LIBS += -L/usr/local/lib -lgdal \
            -L/usr/local/lib -lnetcdf-cxx4


    contains(DEFINES,USE_OPENMP){

        QMAKE_CC = /usr/local/opt/llvm/bin/clang
        QMAKE_CXX = /usr/local/opt/llvm/bin/clang++
        QMAKE_LINK = /usr/local/opt/llvm/bin/clang++

        QMAKE_CFLAGS+= -fopenmp
        QMAKE_LFLAGS+= -fopenmp
        QMAKE_CXXFLAGS+= -fopenmp

        INCLUDEPATH += /usr/local/opt/llvm/lib/clang/5.0.0/include
        LIBS += -L /usr/local/opt/llvm/lib -lomp

      message("OpenMP enabled")
    } else {
      message("OpenMP disabled")
    }


    contains(DEFINES,USE_MPI){

        QMAKE_CC = /usr/local/bin/mpicc
        QMAKE_CXX = /usr/local/bin/mpicxx
        QMAKE_LINK = /usr/local/bin/mpicxx

        QMAKE_CFLAGS += $$system(/usr/local/bin/mpicc --showme:compile)
        QMAKE_CXXFLAGS += $$system(/usr/local/bin/mpic++ --showme:compile)
        QMAKE_LFLAGS += $$system(/usr/local/bin/mpic++ --showme:link)

        LIBS += -L/usr/local/lib/ -lmpi

      message("MPI enabled")
    } else {
      message("MPI disabled")
    }
}

linux{

    INCLUDEPATH += /usr/include \
                   ../gdal/include

    contains(DEFINES,UTAH_CHPC){

         INCLUDEPATH += /uufs/chpc.utah.edu/sys/installdir/hdf5/1.8.17-c7/include \
                        /uufs/chpc.utah.edu/sys/installdir/netcdf-c/4.3.3.1/include \
                        /uufs/chpc.utah.edu/sys/installdir/netcdf-cxx/4.3.0-c7/include


         LIBS += -L/uufs/chpc.utah.edu/sys/installdir/hdf5/1.8.17-c7/lib -l:libhdf5.so.10.2.0 \
                 -L/uufs/chpc.utah.edu/sys/installdir/netcdf-c/4.4.1/lib -l:libnetcdf.so.11.0.3 \
                 -L/uufs/chpc.utah.edu/sys/installdir/netcdf-cxx/4.3.0-c7/lib -l:libnetcdf_c++4.so.1.0.3

         message("Compiling on CHPC")
    }

    contains(DEFINES,USE_OPENMP){

    QMAKE_CFLAGS   += -fopenmp
    QMAKE_LFLAGS   += -fopenmp
    QMAKE_CXXFLAGS += -fopenmp

    LIBS += -L/usr/lib/x86_64-linux-gnu -lgomp

      message("OpenMP enabled")
    } else {
      message("OpenMP disabled")
    }

    contains(DEFINES,USE_MPI){

        QMAKE_CC = mpicc
        QMAKE_CXX = mpic++
        QMAKE_LINK = mpic++

        QMAKE_CFLAGS += $$system(/usr/local/bin/mpicc --showme:compile)
        QMAKE_CXXFLAGS += $$system(/usr/local/bin/mpic++ --showme:compile)
        QMAKE_LFLAGS += $$system(/usr/local/bin/mpic++ --showme:link)

        LIBS += -L/usr/local/lib/ -lmpi

      message("MPI enabled")
    } else {
      message("MPI disabled")
    }
}

win32{

    contains(DEFINES,USE_OPENMP){

        QMAKE_CFLAGS += /openmp
        QMAKE_CXXFLAGS += /openmp
        message("OpenMP enabled")

    } else {

        message("OpenMP disabled")
    }

    #Windows vspkg package manager installation path
    VCPKGDIR = C:/vcpkg/installed/x64-windows

    INCLUDEPATH += $${VCPKGDIR}/include \
                   $${VCPKGDIR}/include/gdal

    message ($$(VCPKGDIR))

    CONFIG(debug, debug|release) {

    LIBS += -L$${VCPKGDIR}/debug/lib -lgdald \
            -L$${VCPKGDIR}/debug/lib -lnetcdf \
            -L$${VCPKGDIR}/debug/lib -lnetcdf-cxx4

            contains(DEFINES,USE_MPI){
               LIBS += -L$${VCPKGDIR}/debug/lib -lmsmpi
               message("MPI enabled")
            } else {
              message("MPI disabled")
            }

        } else {

    LIBS += -L$${VCPKGDIR}/lib -lgdal \
            -L$${VCPKGDIR}/lib -lnetcdf \
            -L$${VCPKGDIR}/lib -lnetcdf-cxx4

            contains(DEFINES,USE_MPI){
               LIBS += -L$${VCPKGDIR}/lib -lmsmpi
               message("MPI enabled")
            } else {
              message("MPI disabled")
            }
    }

    QMAKE_CXXFLAGS += /MP
    QMAKE_LFLAGS += /incremental /debug:fastlink
}


CONFIG(debug, debug|release) {

    win32 {
       QMAKE_CXXFLAGS += /MDd /O2
    }

    macx {
#       QMAKE_CXXFLAGS += -O3
    }

    linux {
       QMAKE_CXXFLAGS += -O3
    }


   DESTDIR = ./build/debug
   OBJECTS_DIR = $$DESTDIR/.obj
   MOC_DIR = $$DESTDIR/.moc
   RCC_DIR = $$DESTDIR/.qrc
   UI_DIR = $$DESTDIR/.ui

   macx{

       QMAKE_POST_LINK += "cp -a ./../HydroCoupleSDK/build/debug/*HydroCoupleSDK.* ./build/debug/";
       QMAKE_POST_LINK += "cp -a ./../SWMM/build/debug/*SWMM.* ./build/debug/";

       LIBS += -L./../HydroCoupleSDK/build/debug -lHydroCoupleSDK.1.0.0
       LIBS += -L./../SWMM/build/debug -lSWMM.$$SWMM_VERSION
    }

   linux{

       QMAKE_POST_LINK += "cp -a ./../HydroCoupleSDK/build/debug/*HydroCoupleSDK.* ./build/debug/";
       QMAKE_POST_LINK += "cp -a ./../SWMM/build/debug/*SWMM.* ./build/debug/";

       LIBS += -L./../HydroCoupleSDK/build/debug -l:libHydroCoupleSDK.so.1.0.0
       LIBS += -L./../SWMM/build/debug -l:libSWMM.so.$$SWMM_VERSION
    }

   win32{

       QMAKE_POST_LINK += "copy /B .\..\HydroCoupleSDK\build\debug\HydroCoupleSDK* .\build\debug &&"
       QMAKE_POST_LINK += "copy /B .\..\SWMM\build\debug\SWMM* .\build\debug"

       LIBS += -L./../HydroCoupleSDK/build/debug -lHydroCoupleSDK1
       LIBS += -L./../SWMM/build/debug -lSWMM5
    }
}

CONFIG(release, debug|release) {

    RELEASE_EXTRAS = ./build/release
    OBJECTS_DIR = $$RELEASE_EXTRAS/.obj
    MOC_DIR = $$RELEASE_EXTRAS/.moc
    RCC_DIR = $$RELEASE_EXTRAS/.qrc
    UI_DIR = $$RELEASE_EXTRAS/.ui


   win32 {
     QMAKE_CXXFLAGS += /MD
   }

   macx{
        LIBS += -L./../HydroCoupleSDK/lib/macx -lHydroCoupleSDK.1.0.0
        LIBS += -L./../SWMM/lib/macx -lSWMM.$$SWMM_VERSION
    }

   linux{
        LIBS += -L./../HydroCoupleSDK/lib/linux -l:libHydroCoupleSDK.so.1.0.0
        LIBS += -L./../SWMM/lib/linux -l:libSWMM.so.$$SWMM_VERSION
    }

   win32{
        LIBS += -L./../HydroCoupleSDK/lib/win32 -lHydroCoupleSDK1
        LIBS += -L./../SWMM/lib/win32 -lSWMM5
    }

     contains(DEFINES,SWMMCOMPONENT_LIBRARY){
         #MacOS
         macx{
             DESTDIR = lib/macx
             QMAKE_POST_LINK += "cp -a ./../HydroCoupleSDK/lib/macx/*HydroCoupleSDK* ./lib/macx/";
             QMAKE_POST_LINK += "cp -a ./../SWMM/lib/macx/*SWMM* ./lib/macx/";
        }

         #Linux
         linux{
             DESTDIR = lib/linux
             QMAKE_POST_LINK += "cp -a ./../HydroCoupleSDK/lib/linux/*HydroCoupleSDK* ./lib/linux/";
             QMAKE_POST_LINK += "cp ./../SWMM/lib/linux/*SWMM* ./lib/linux/";
        }

         #Windows
         win32{
             DESTDIR = lib/win32
             QMAKE_POST_LINK += "copy /B .\..\HydroCoupleSDK\lib\win32\HydroCoupleSDK* .\lib\win32 &&"
             QMAKE_POST_LINK += "copy /B .\..\SWMM\lib\win32\SWMM* .\lib\win32"
        }
    } else {
         #MacOS
         macx{
             DESTDIR = bin/macx
             QMAKE_POST_LINK += "cp -a ./../HydroCoupleSDK/lib/macx/*HydroCoupleSDK* ./bin/macx/";
             QMAKE_POST_LINK += "cp -a ./../SWMM/lib/macx/*SWMM* ./bin/macx/";
        }

         #Linux
         linux{
             DESTDIR = bin/linux
             QMAKE_POST_LINK += "cp -a ./../HydroCoupleSDK/lib/linux/*HydroCoupleSDK* ./bin/linux/";
             QMAKE_POST_LINK += "cp -a ./../SWMM/lib/linux/*SWMM* ./bin/linux/";
        }

         #Windows
         win32{
             DESTDIR = bin/win32
             QMAKE_POST_LINK += "copy /B .\..\HydroCoupleSDK\lib\win32\HydroCoupleSDK* .\bin\win32 &&"
             QMAKE_POST_LINK += "copy /B .\..\SWMM\lib\win32\SWMM* .\bin\win32"
        }
    }
}

RESOURCES += \
    resources/swmmcomponent.qrc
