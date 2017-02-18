
#---NIFTI-REC---
INCLUDEPATH += D:/NiftyRec/SOURCE/emission-lib
INCLUDEPATH += D:/NiftyRec/SOURCE/reg-lib
INCLUDEPATH += D:/NiftyRec/SOURCE/nifti

LIBS += -LD:/NiftyRec/binary32bit/emission-lib/Release/ -l_et -l_et_accumulate -l_et_array_interface -l_et_clear_accumulator -l_et_common -l_et_convolve2D -l_et_convolveFFT2D -l_et_convolveSeparable2D -l_et_line_backproject -l_et_line_backproject_attenuated -l_et_line_integral -l_et_line_integral_attenuated -l_niftyrec_memory
LIBS += -LD:/NiftyRec/binary32bit/reg-lib/Release/ -l_reg -l_reg_affineTransformation -l_reg_array_interface -l_reg_blockMatching -l_reg_bspline -l_reg_mutualinformation -l_reg_resampling -l_reg_ssd -l_reg_tools
LIBS += -LD:/NiftyRec/binary32bit/nifti/Release -lreg_nifti
#-----------

#---CERN ROOT---
win32 {
        INCLUDEPATH += c:/root/include
        LIBS += -Lc:/root/lib/ -llibCore -llibCint -llibRIO -llibNet -llibHist -llibGraf -llibGraf3d -llibGpad -llibTree -llibRint -llibPostscript -llibMatrix -llibPhysics -llibRint -llibMathCore -llibGeom -llibGeomPainter -llibGeomBuilder -llibMathMore -llibMinuit2 -llibThread
}
linux-g++ || unix {
        INCLUDEPATH += $$system(root-config --incdir)
        LIBS += $$system(root-config --libs) -lGeom -lGeomPainter -lGeomBuilder -lMathMore -lMinuit2
}
#-----------

QT       += websockets core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NiftySpect-01
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    mywebserver.cpp \
    arootmanager.cpp \
    utils.cpp


HEADERS  += mainwindow.h \
    mywebserver.h \
    arootmanager.h \
    utils.h

FORMS    += mainwindow.ui

win32 {
  DEFINES  += _CRT_SECURE_NO_WARNINGS   #disable microsoft spam
}

