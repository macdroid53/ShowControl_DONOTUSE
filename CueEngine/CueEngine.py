#!/usr/bin/env python3
__author__ = 'mac'

import sys
import types
import argparse

from PyQt5 import Qt, QtCore, QtGui, QtWidgets
import xml.etree.ElementTree as ET
from os import path

from ShowConf import ShowConf
from Cues import CueList
from UDPClient import *

import CueEngine_ui
#import mainwindow
from pythonosc import osc_message_builder
#from pythonosc import udp_client

import styles

class Show:
    '''
    The Show class contains the information and object that constitute a show
    '''
    def __init__(self, show_confpath):
        '''
        Constructor
        '''
        self.show_conf = ShowConf(show_confpath + 'ShowConf.xml')
        self.cues = CueList(show_confpath + 'Scrooge Moves.xml')
        self.cues.currentcueindex = 0
        self.cues.setcurrentcuestate(self.cues.currentcueindex)

class CueDlg(QtWidgets.QMainWindow, CueEngine_ui.Ui_MainWindow):

    def __init__(self, cuelistfile, parent=None):
        super(CueDlg, self).__init__(parent)
        QtGui.QIcon.setThemeSearchPaths(styles.QLiSPIconsThemePaths)
        QtGui.QIcon.setThemeName(styles.QLiSPIconsThemeName)
        self.__index = 0
        self.setupUi(self)
        self.nextButton.clicked.connect(self.on_buttonNext_clicked)
        self.prevButton.clicked.connect(self.on_buttonPrev_clicked)

    def on_buttonNext_clicked(self):
        print('Next')

        previdx = The_Show.cues.currentcueindex
        The_Show.cues.currentcueindex += 1
        tblvw = self.findChild(QtWidgets.QTableView)
        tblvw.selectRow(The_Show.cues.currentcueindex)
        print('Old index: ' + str(previdx) + '   New: ' + str(The_Show.cues.currentcueindex))
        The_Show.cues.setcurrentcuestate(The_Show.cues.currentcueindex)

        print('/cue ' + ascii(The_Show.cues.currentcueindex))
        msg = '/cue ' + ascii(The_Show.cues.currentcueindex)
        #msg = osc_message_builder.OscMessageBuilder(address='/cue')
        #msg.add_arg('{0:03}'.format(The_Show.cues.currentcueindex))
        #msg = msg.build()

        sender.send(msg)

    def on_buttonPrev_clicked(self):
        print('Prev')
        if The_Show.cues.currentcueindex > 0:

            previdx = The_Show.cues.currentcueindex
            The_Show.cues.currentcueindex -= 1
            print('Old index: ' + str(previdx) + '   New: ' + str(The_Show.cues.currentcueindex))
            The_Show.cues.setcurrentcuestate(The_Show.cues.currentcueindex)
        else:
            The_Show.cues.currentcueindex = 0
        tblvw = self.findChild(QtWidgets.QTableView)
        tblvw.selectRow(The_Show.cues.currentcueindex)

#            msg = msg.build()
#            client.send(msg)

    def setfirstcue(self):
        tblvw = self.findChild(QtWidgets.QTableView)
        tblvw.selectRow(The_Show.cues.currentcueindex)

    def disptext(self):
        self.get_table_data()
        # set the table model
        header = ['Cue', 'Id', 'Act', 'Scene', 'Page','Title','Dialog/Prompt']
        tablemodel = MyTableModel(self.tabledata, header, self)
        self.tableView.setModel(tablemodel)
        self.tableView.resizeColumnsToContents()

    def get_table_data(self):
        qs = The_Show.cues.cuelist.findall('cue')
        self.tabledata =[]
        for q in qs:
            #print(q.attrib)
            #print(q.find('Move').text)
            self.tabledata.append([q.find('Move').text,
                     q.find('Id').text,
                     q.find('Scene').text,
                     q.find('Page').text,
                     q.find('Title').text,
                     q.find('Cue').text])
        #print(self.tabledata)

class MyTableModel(QtCore.QAbstractTableModel):
    def __init__(self, datain, headerdata, parent=None):
        """
        Args:
            datain: a list of lists\n
            headerdata: a list of strings
        """
        QtCore.QAbstractTableModel.__init__(self, parent)
        self.arraydata = datain
        self.headerdata = headerdata

    def rowCount(self, parent):
        return len(self.arraydata)

    def columnCount(self, parent):
        if len(self.arraydata) > 0:
            return len(self.arraydata[0])
        return 0

    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        elif role != QtCore.Qt.DisplayRole:
            return QtCore.QVariant()
        return QtCore.QVariant(self.arraydata[index.row()][index.column()])

    def setData(self, index, value, role):
        pass         # not sure what to put here

    def headerData(self, col, orientation, role):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return QtCore.QVariant(self.headerdata[col])
        return QtCore.QVariant()

    def sort(self, Ncol, order):
        """
        Sort table by given column number.
        """
        self.emit(SIGNAL("layoutAboutToBeChanged()"))
        self.arraydata = sorted(self.arraydata, key=operator.itemgetter(Ncol))
        if order == QtCore.Qt.DescendingOrder:
            self.arraydata.reverse()
        self.emit(SIGNAL("layoutChanged()"))




The_Show = Show(path.abspath(path.join(path.dirname(__file__))) + '/')


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
#     app.setStyleSheet(""" QPushButton {color: blue;
#                          background-color: yellow;
#                          selection-color: blue;
#                          selection-background-color: green;}""")
    #app.setStyleSheet("QPushButton {pressed-color: red }")
    app.setStyleSheet(styles.QLiSPTheme_Dark)
    ui = CueDlg(path.abspath(path.join(path.dirname(__file__))) + '/Scrooge Moves.xml')
    ui.resize(800,800)
    ui.disptext()
    ui.setfirstcue()
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", default="127.0.0.1", help="The ip of the OSC server")
    parser.add_argument("--port", type=int, default=5005, help="The port the OSC server is listening on")
    args = parser.parse_args()

    sender = UDPClient(args.ip, args.port)
    #ui.set_scribble(The_Show.chrchnmap.maplist)
    #ui.initmutes()
    #tblvw = ui.findChild(QtWidgets.QTableView)
    #tblvw.selectRow(The_Show.cues.currentcueindex)
    ui.show()
    sys.exit(app.exec_())