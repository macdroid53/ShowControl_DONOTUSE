# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'CueEngine-2.ui'
#
# Created by: PyQt5 UI code generator 5.7
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.resize(850, 768)
        self.centralwidget = QtWidgets.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        self.horizontalLayout_2 = QtWidgets.QHBoxLayout(self.centralwidget)
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.verticalLayout = QtWidgets.QVBoxLayout()
        self.verticalLayout.setObjectName("verticalLayout")
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.nextButton = QtWidgets.QPushButton(self.centralwidget)
        self.nextButton.setMinimumSize(QtCore.QSize(64, 64))
        self.nextButton.setObjectName("nextButton")
        self.horizontalLayout.addWidget(self.nextButton)
        self.prevButton = QtWidgets.QPushButton(self.centralwidget)
        self.prevButton.setMinimumSize(QtCore.QSize(64, 64))
        self.prevButton.setObjectName("prevButton")
        self.horizontalLayout.addWidget(self.prevButton)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.tableView = QtWidgets.QTableView(self.centralwidget)
        self.tableView.setObjectName("tableView")
        self.verticalLayout.addWidget(self.tableView)
        self.horizontalLayout_3 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_3.setObjectName("horizontalLayout_3")
        spacerItem1 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout_3.addItem(spacerItem1)
        self.quitButton = QtWidgets.QPushButton(self.centralwidget)
        self.quitButton.setMinimumSize(QtCore.QSize(64, 64))
        self.quitButton.setObjectName("quitButton")
        self.horizontalLayout_3.addWidget(self.quitButton)
        self.verticalLayout.addLayout(self.horizontalLayout_3)
        self.horizontalLayout_2.addLayout(self.verticalLayout)
        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtWidgets.QMenuBar(MainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 850, 28))
        self.menubar.setObjectName("menubar")
        self.menu_File = QtWidgets.QMenu(self.menubar)
        self.menu_File.setObjectName("menu_File")
        self.menu_Edit = QtWidgets.QMenu(self.menubar)
        self.menu_Edit.setObjectName("menu_Edit")
        self.menu_View = QtWidgets.QMenu(self.menubar)
        self.menu_View.setObjectName("menu_View")
        self.menu_Application = QtWidgets.QMenu(self.menubar)
        self.menu_Application.setObjectName("menu_Application")
        MainWindow.setMenuBar(self.menubar)
        self.statusbar = QtWidgets.QStatusBar(MainWindow)
        self.statusbar.setObjectName("statusbar")
        MainWindow.setStatusBar(self.statusbar)
        self.toolBar = QtWidgets.QToolBar(MainWindow)
        self.toolBar.setObjectName("toolBar")
        MainWindow.addToolBar(QtCore.Qt.TopToolBarArea, self.toolBar)
        self.actionOpen_Show = QtWidgets.QAction(MainWindow)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap("Open.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.actionOpen_Show.setIcon(icon)
        self.actionOpen_Show.setObjectName("actionOpen_Show")
        self.action_Stage_Cues = QtWidgets.QAction(MainWindow)
        self.action_Stage_Cues.setCheckable(True)
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap("Stage.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_Stage_Cues.setIcon(icon1)
        self.action_Stage_Cues.setObjectName("action_Stage_Cues")
        self.action_Sound_Cues = QtWidgets.QAction(MainWindow)
        self.action_Sound_Cues.setCheckable(True)
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap("Sound.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_Sound_Cues.setIcon(icon2)
        self.action_Sound_Cues.setObjectName("action_Sound_Cues")
        self.action_Lighting_Cues = QtWidgets.QAction(MainWindow)
        self.action_Lighting_Cues.setCheckable(True)
        icon3 = QtGui.QIcon()
        icon3.addPixmap(QtGui.QPixmap("Lighting.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_Lighting_Cues.setIcon(icon3)
        self.action_Lighting_Cues.setObjectName("action_Lighting_Cues")
        self.action_Lighting = QtWidgets.QAction(MainWindow)
        self.action_Lighting.setCheckable(True)
        icon4 = QtGui.QIcon()
        icon4.addPixmap(QtGui.QPixmap("LightingApp.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        icon4.addPixmap(QtGui.QPixmap("LightingApp.png"), QtGui.QIcon.Normal, QtGui.QIcon.On)
        self.action_Lighting.setIcon(icon4)
        self.action_Lighting.setObjectName("action_Lighting")
        self.action_Mixer = QtWidgets.QAction(MainWindow)
        self.action_Mixer.setCheckable(True)
        icon5 = QtGui.QIcon()
        icon5.addPixmap(QtGui.QPixmap("MixerApp.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_Mixer.setIcon(icon5)
        self.action_Mixer.setObjectName("action_Mixer")
        self.actionExit = QtWidgets.QAction(MainWindow)
        self.actionExit.setObjectName("actionExit")
        self.actionSave = QtWidgets.QAction(MainWindow)
        self.actionSave.setObjectName("actionSave")
        self.actionPreferences = QtWidgets.QAction(MainWindow)
        self.actionPreferences.setObjectName("actionPreferences")
        self.actionClose_Show = QtWidgets.QAction(MainWindow)
        self.actionClose_Show.setObjectName("actionClose_Show")
        self.action_Sound_FX = QtWidgets.QAction(MainWindow)
        self.action_Sound_FX.setCheckable(True)
        icon6 = QtGui.QIcon()
        icon6.addPixmap(QtGui.QPixmap("SoundFX.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_Sound_FX.setIcon(icon6)
        self.action_Sound_FX.setObjectName("action_Sound_FX")
        self.menu_File.addAction(self.actionOpen_Show)
        self.menu_File.addAction(self.actionClose_Show)
        self.menu_File.addAction(self.actionSave)
        self.menu_File.addAction(self.actionExit)
        self.menu_Edit.addAction(self.actionPreferences)
        self.menu_View.addAction(self.action_Stage_Cues)
        self.menu_View.addAction(self.action_Sound_Cues)
        self.menu_View.addAction(self.action_Lighting_Cues)
        self.menu_Application.addAction(self.action_Lighting)
        self.menu_Application.addAction(self.action_Mixer)
        self.menu_Application.addAction(self.action_Sound_FX)
        self.menubar.addAction(self.menu_File.menuAction())
        self.menubar.addAction(self.menu_Edit.menuAction())
        self.menubar.addAction(self.menu_View.menuAction())
        self.menubar.addAction(self.menu_Application.menuAction())
        self.toolBar.addAction(self.actionOpen_Show)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.action_Stage_Cues)
        self.toolBar.addAction(self.action_Sound_Cues)
        self.toolBar.addAction(self.action_Lighting_Cues)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.action_Lighting)
        self.toolBar.addAction(self.action_Mixer)
        self.toolBar.addAction(self.action_Sound_FX)

        self.retranslateUi(MainWindow)
        self.actionExit.triggered.connect(MainWindow.close)
        self.quitButton.clicked.connect(MainWindow.close)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        _translate = QtCore.QCoreApplication.translate
        MainWindow.setWindowTitle(_translate("MainWindow", "Cue Engine"))
        self.nextButton.setText(_translate("MainWindow", "Next"))
        self.prevButton.setText(_translate("MainWindow", "Previous"))
        self.quitButton.setText(_translate("MainWindow", "Quit"))
        self.menu_File.setTitle(_translate("MainWindow", "Fi&le"))
        self.menu_Edit.setTitle(_translate("MainWindow", "E&dit"))
        self.menu_View.setTitle(_translate("MainWindow", "&View"))
        self.menu_Application.setTitle(_translate("MainWindow", "Applicatio&n"))
        self.toolBar.setWindowTitle(_translate("MainWindow", "toolBar"))
        self.actionOpen_Show.setText(_translate("MainWindow", "&Open Show"))
        self.action_Stage_Cues.setText(_translate("MainWindow", "&Stage Cues"))
        self.action_Stage_Cues.setToolTip(_translate("MainWindow", "Display Stage Cues"))
        self.action_Sound_Cues.setText(_translate("MainWindow", "S&ound Cues"))
        self.action_Sound_Cues.setToolTip(_translate("MainWindow", "Display Sound Cues"))
        self.action_Lighting_Cues.setText(_translate("MainWindow", "&Lighting Cues"))
        self.action_Lighting_Cues.setToolTip(_translate("MainWindow", "Display Lighting Cues"))
        self.action_Lighting.setText(_translate("MainWindow", "&Lighting"))
        self.action_Mixer.setText(_translate("MainWindow", "&Mixer"))
        self.actionExit.setText(_translate("MainWindow", "&Exit"))
        self.actionSave.setText(_translate("MainWindow", "&Save"))
        self.actionPreferences.setText(_translate("MainWindow", "&Preferences"))
        self.actionClose_Show.setText(_translate("MainWindow", "&Close Show"))
        self.action_Sound_FX.setText(_translate("MainWindow", "&Sound FX"))

