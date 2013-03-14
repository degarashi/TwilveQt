#-------------------------------------------------
#
# Project created by QtCreator 2013-01-21T16:48:28
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TwilveQt
TEMPLATE = app


SOURCES += main.cpp\
	oauth.cpp \
	sha1.cpp \
	twqNet.cpp \
	api_kwd.cpp \
	validation.cpp \
	twqMain.cpp \
	twquser.cpp \
	twqtweet.cpp \
	common.cpp \
	dlg_tweet.cpp \
	twqcell.cpp \
	twqtl.cpp \
	dlg_user.cpp \
	twqsearchtl.cpp \
	twqhometl.cpp \
	twqmentionstl.cpp \
	twqusertl.cpp \
	dlg_addsearch.cpp \
	twqresource.cpp

HEADERS  += \
	twqNet.h \
	oauth.h \
	sha1.h \
	common.h \
	api_kwd.h \
	json_kwd.h \
	validation.h \
	twqMain.h \
	twquser.h \
	twqtweet.h \
	type.h \
	dlg_tweet.h \
	twqcell.h \
	twqtl.h \
	dlg_user.h \
	twqtlvariant.h \
	dlg_addsearch.h \
	twqresource.h

FORMS += \
	mainwindow.ui \
	validation.ui \
	dlg_tweet.ui \
	dlg_user.ui \
	dlg_addsearch.ui
TRANSLATIONS = tr_ja.ts

#INCLUDEPATH += /usr/local/include/c++/4.7.2
#INCLUDEPATH += /usr/local/include/c++/4.7.2/i686-pc-linux-gnu
QMAKE_CXXFLAGS += -std=c++11 -Wno-unused-parameter -Wno-unused-variable
QMAKE_CXX = g++-4.72
QMAKE_LINK = g++-4.72