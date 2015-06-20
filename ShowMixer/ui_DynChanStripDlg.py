# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'DynChanStripDlg.ui'
#
# Created: Sun Apr 26 14:04:22 2015
#      by: PyQt5 UI code generator 5.2.1
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_stripDialog(object):
    def setupUi(self, stripDialog):
        stripDialog.setObjectName("stripDialog")
        stripDialog.resize(903, 797)
        self.verticalLayout = QtWidgets.QVBoxLayout(stripDialog)
        self.verticalLayout.setObjectName("verticalLayout")
        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem)
        self.stripgridLayout = QtWidgets.QGridLayout()
        self.stripgridLayout.setObjectName("stripgridLayout")
        self.verticalLayout.addLayout(self.stripgridLayout)
        spacerItem1 = QtWidgets.QSpacerItem(20, 200, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem1)
        self.butonsLayout = QtWidgets.QHBoxLayout()
        self.butonsLayout.setObjectName("butonsLayout")
        spacerItem2 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.butonsLayout.addItem(spacerItem2)
        self.quitButton = QtWidgets.QPushButton(stripDialog)
        self.quitButton.setObjectName("quitButton")
        self.butonsLayout.addWidget(self.quitButton)
        self.verticalLayout.addLayout(self.butonsLayout)

        self.retranslateUi(stripDialog)
        self.quitButton.clicked.connect(self.quitButton.close)
        QtCore.QMetaObject.connectSlotsByName(stripDialog)

    def retranslateUi(self, stripDialog):
        _translate = QtCore.QCoreApplication.translate
        stripDialog.setWindowTitle(_translate("stripDialog", "Channel Strip Dialog"))
        self.quitButton.setText(_translate("stripDialog", "Quit"))

