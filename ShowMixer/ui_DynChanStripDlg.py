# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '/home/mac/workspace/ChanStrips/ChanStrips-1/DynChanStripDlg.ui'
#
# Created: Sun Sep 28 10:30:34 2014
#      by: PyQt4 UI code generator 4.10.4
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtWidgets.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtWidgets.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtWidgets.QApplication.translate(context, text, disambig)

class Ui_stripDialog(object):
    def setupUi(self, stripDialog):
        stripDialog.setObjectName(_fromUtf8("stripDialog"))
        stripDialog.resize(903, 797)
        self.verticalLayout = QtWidgets.QVBoxLayout(stripDialog)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem)
        self.stripgridLayout = QtWidgets.QGridLayout()
        self.stripgridLayout.setObjectName(_fromUtf8("stripgridLayout"))
        self.verticalLayout.addLayout(self.stripgridLayout)
        spacerItem1 = QtWidgets.QSpacerItem(20, 200, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem1)
        self.butonsLayout = QtWidgets.QHBoxLayout()
        self.butonsLayout.setObjectName(_fromUtf8("butonsLayout"))
        spacerItem2 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.butonsLayout.addItem(spacerItem2)
        self.quitButton = QtWidgets.QPushButton(stripDialog)
        self.quitButton.setObjectName(_fromUtf8("quitButton"))
        self.butonsLayout.addWidget(self.quitButton)
        self.verticalLayout.addLayout(self.butonsLayout)

        self.retranslateUi(stripDialog)
        #self.quitButton.(stripDialog.reject)
        self.quitButton.clicked.connect(stripDialog.reject)
        #QMetaObject.connectSlotsByName(stripDialog)

    def retranslateUi(self, stripDialog):
        stripDialog.setWindowTitle(_translate("stripDialog", "Channel Strip Dialog", None))
        self.quitButton.setText(_translate("stripDialog", "Quit", None))

