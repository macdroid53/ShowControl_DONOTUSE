# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'preferences.ui'
#
# Created: Sun Jul  5 14:04:26 2015
#      by: PyQt5 UI code generator 5.2.1
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_Preferences(object):
    def setupUi(self, Preferences):
        Preferences.setObjectName("Preferences")
        Preferences.resize(400, 300)
        self.buttonBox = QtWidgets.QDialogButtonBox(Preferences)
        self.buttonBox.setGeometry(QtCore.QRect(30, 240, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtWidgets.QDialogButtonBox.Cancel|QtWidgets.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.cbxExitwCueEngine = QtWidgets.QCheckBox(Preferences)
        self.cbxExitwCueEngine.setGeometry(QtCore.QRect(30, 30, 151, 22))
        self.cbxExitwCueEngine.setObjectName("cbxExitwCueEngine")

        self.retranslateUi(Preferences)
        self.buttonBox.accepted.connect(Preferences.accept)
        self.buttonBox.rejected.connect(Preferences.reject)
        QtCore.QMetaObject.connectSlotsByName(Preferences)

    def retranslateUi(self, Preferences):
        _translate = QtCore.QCoreApplication.translate
        Preferences.setWindowTitle(_translate("Preferences", "Preferences"))
        self.cbxExitwCueEngine.setText(_translate("Preferences", "Exit with Cue Engine"))

