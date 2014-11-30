# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'mainwindow.ui'
#
# Created: Sun Nov  2 10:37:24 2014
#      by: PyQt5 UI code generator 5.2.1
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_stripDialog(object):
    def setupUi(self, stripDialog):
        stripDialog.setObjectName("stripDialog")
        stripDialog.resize(903, 797)
        self.centralWidget = QtWidgets.QWidget(stripDialog)
        self.centralWidget.setObjectName("centralWidget")
        self.verticalLayout = QtWidgets.QVBoxLayout(self.centralWidget)
        self.verticalLayout.setObjectName("verticalLayout")
        spacerItem = QtWidgets.QSpacerItem(20, 349, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem)
        self.stripgridLayout = QtWidgets.QGridLayout()
        self.stripgridLayout.setObjectName("stripgridLayout")
        self.verticalLayout.addLayout(self.stripgridLayout)
        spacerItem1 = QtWidgets.QSpacerItem(20, 348, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem1)
        self.butonsLayout = QtWidgets.QHBoxLayout()
        self.butonsLayout.setObjectName("butonsLayout")
        spacerItem2 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.butonsLayout.addItem(spacerItem2)
        self.quitButton = QtWidgets.QPushButton(self.centralWidget)
        self.quitButton.setObjectName("quitButton")
        self.butonsLayout.addWidget(self.quitButton)
        self.verticalLayout.addLayout(self.butonsLayout)
        stripDialog.setCentralWidget(self.centralWidget)
        self.menuBar = QtWidgets.QMenuBar(stripDialog)
        self.menuBar.setGeometry(QtCore.QRect(0, 0, 903, 27))
        self.menuBar.setNativeMenuBar(True)
        self.menuBar.setObjectName("menuBar")
        self.menu_File = QtWidgets.QMenu(self.menuBar)
        self.menu_File.setObjectName("menu_File")
        stripDialog.setMenuBar(self.menuBar)
        self.action_Exit = QtWidgets.QAction(stripDialog)
        self.action_Exit.setObjectName("action_Exit")
        self.menu_File.addAction(self.action_Exit)
        self.menuBar.addAction(self.menu_File.menuAction())

        self.retranslateUi(stripDialog)
        self.quitButton.clicked.connect(stripDialog.close)
        self.action_Exit.triggered.connect(stripDialog.close)
        QtCore.QMetaObject.connectSlotsByName(stripDialog)

    def retranslateUi(self, stripDialog):
        _translate = QtCore.QCoreApplication.translate
        stripDialog.setWindowTitle(_translate("stripDialog", "MainWindow"))
        self.quitButton.setText(_translate("stripDialog", "Quit"))
        self.menu_File.setTitle(_translate("stripDialog", "&File"))
        self.action_Exit.setText(_translate("stripDialog", "&Exit"))

