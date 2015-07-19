#!/usr/bin/env python3
__author__ = 'mac'

import os, sys, inspect
import types
import argparse

from PyQt5 import Qt, QtCore, QtGui, QtWidgets
from PyQt5.QtCore import *
from PyQt5.QtWidgets import *

import xml.etree.ElementTree as ET
from os import path

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
print(currentdir)
syblingdir =  os.path.dirname(currentdir) + '/ShowControl/utils'
print(syblingdir)
parentdir = os.path.dirname(currentdir)
print(parentdir)
sys.path.insert(0,syblingdir)
print(sys.path)

from ShowConf import ShowConf
from Cues import CueList
#from MixerConf import MixerConf
from UDPClient import *

import CueEngine_ui
from CueEdit_ui import Ui_dlgEditCue
#import mainwindow
from pythonosc import osc_message_builder
#from pythonosc import udp_client

import configuration as cfg

import styles

cfgdict = cfg.toDict()

columndict = {'Number': 0, 'Act':1, 'Scene':2, 'Page':3, 'ID':4, 'Title':5,'Dialog/Prompt':6}

class EditCue(QDialog, Ui_dlgEditCue):
    def __init__(self, index, parent=None):
        QDialog.__init__(self, parent)
        #super(object, self).__init__(self)
        self.editidx = index
        self.setupUi(self)
        self.chgdict = {}
        #self.changeflag = False
        #self.plainTextEditAct.textChanged.connect(self.setChangeFlag)

    def accept(self):
        somethingchanged = False
        for dobj in self.findChildren(QtWidgets.QPlainTextEdit):
            tobj = dobj.document()
            if tobj.isModified():
                somethingchanged = True
            #print(dobj)
        if somethingchanged:
            print('editidx',self.editidx)
            for dobj in self.findChildren(QtWidgets.QPlainTextEdit):
                objnam = dobj.objectName()
                flddoc = dobj.documentTitle()
                print('documentTitle: ', flddoc)
                print('object name: ', objnam)
                fldtxt = dobj.toPlainText()
                self.chgdict.update({flddoc:fldtxt})

        print('Something changed: ', somethingchanged)
        docobj = self.plainTextEditTitle.document()
        print('Window modded:',self.isWindowModified())
        #print(docobj.isModified())
        # if docobj.isModified():
        #     print(self.plainTextEditTitle.toPlainText())
        #     self.chgdict.update()
        #     #save changes
        #     #save to cue file
        #     #redisplay
        # self.chglist.append('ddd')
        # #rowlist = self.sender().tableview #.tabledata[self.editidx]
        # #print('rowlist', rowlist)
        super(EditCue, self).accept()

    def reject(self):
        super(EditCue, self).reject()

    def getchange(self):
        return self.chglist

    #def setChangeFlag(self):
    #    print('textchanged')
    #    self.changeflag = True


class Show:
    '''
    The Show class contains the information and object that constitute a show
    '''
    def __init__(self):
        '''
        Constructor
        '''
        self.show_confpath = cfgdict['Show']['folder'] + '/'
        self.show_conf = ShowConf(self.show_confpath + cfgdict['Show']['file'])
        self.cues = CueList(self.show_confpath + self.show_conf.settings['mxrcue'])
        self.cues.currentcueindex = 0
        self.cues.setcurrentcuestate(self.cues.currentcueindex)

    def loadNewShow(self, newpath):
        '''
            :param sho_configpath: path to new ShowConf.xml
            :return:
        '''
        print(cfgdict)
        self.show_confpath, showfile = path.split(newpath)
        #self.show_confpath = path.dirname(newpath)
        self.show_confpath = self.show_confpath + '/'
        cfgdict['Show']['folder'] = self.show_confpath
        cfgdict['Show']['file'] = showfile
        cfg.updateFromDict(cfgdict)
        cfg.write()
        self.show_conf = ShowConf(self.show_confpath + cfgdict['Show']['file'])
        self.cues = CueList(self.show_confpath + self.show_conf.settings['mxrcue'])
        self.cues.currentcueindex = 0
        self.cues.setcurrentcuestate(self.cues.currentcueindex)
        self.displayShow()

    def displayShow(self):
        '''
        Update the state of the mixer display to reflect the newly loaded show
        '''
        #print(self.cues)
        qs = self.cues.cuelist.findall('cue')
        for q in qs:
             print(q.attrib)


class CueDlg(QtWidgets.QMainWindow, CueEngine_ui.Ui_MainWindow):

    def __init__(self, cuelistfile, parent=None):
        super(CueDlg, self).__init__(parent)
        QtGui.QIcon.setThemeSearchPaths(styles.QLiSPIconsThemePaths)
        QtGui.QIcon.setThemeName(styles.QLiSPIconsThemeName)
        self.__index = 0
        self.setupUi(self)
        self.setWindowTitle(The_Show.show_conf.settings['name'])
        self.nextButton.clicked.connect(self.on_buttonNext_clicked)
        self.prevButton.clicked.connect(self.on_buttonPrev_clicked)
        self.tableView.doubleClicked.connect(self.on_table_click)
        self.actionOpen_Show.triggered.connect(self.openShow)
        self.editcuedlg = EditCue('0')


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
        print('/cue ' + ascii(The_Show.cues.currentcueindex))
        msg = '/cue ' + ascii(The_Show.cues.currentcueindex)
        tblvw = self.findChild(QtWidgets.QTableView)
        tblvw.selectRow(The_Show.cues.currentcueindex)
        sender.send(msg)

    def on_table_click(self,index):
        print(index.row())
        self.editcuedlg.editidx = index.row()
        print(self.tabledata[index.row()])
        rowlist = self.tabledata[index.row()]
        self.editcuedlg.plainTextEditCueNum.setPlainText(rowlist[0])
        self.editcuedlg.plainTextEditCueNum.setDocumentTitle('Cue Number')

        self.editcuedlg.plainTextEditAct.setPlainText(rowlist[1])
        self.editcuedlg.plainTextEditAct.setDocumentTitle('Act')

        self.editcuedlg.plainTextEditScene.setPlainText(rowlist[2])
        self.editcuedlg.plainTextEditScene.setDocumentTitle('Scene')

        self.editcuedlg.plainTextEditPage.setPlainText(rowlist[3])
        self.editcuedlg.plainTextEditPage.setDocumentTitle('Page')

        self.editcuedlg.plainTextEditId.setPlainText(rowlist[4])
        self.editcuedlg.plainTextEditId.setDocumentTitle('ID')

        self.editcuedlg.plainTextEditTitle.setPlainText(rowlist[5])
        self.editcuedlg.plainTextEditTitle.setDocumentTitle('Title')

        self.editcuedlg.plainTextEditPrompt.setPlainText(rowlist[6])
        self.editcuedlg.plainTextEditPrompt.setDocumentTitle('Dialog/Prompt')

        #self.editcuedlg.show()
        self.editcuedlg.exec_()
        changedict = self.editcuedlg.chgdict
        print('returned list:',self.editcuedlg.chgdict)
        if changedict:
            print('Updating table.')
            for key, newdata in changedict.items():
                print('--------------',key,newdata)
                for coltxt, colidx in columndict.items():
                    if coltxt in key:
                        print('colidx: ', colidx, ' row: ', index.row())
                        print('coltxt: ',coltxt, ' newdata: ',newdata)
                        self.tabledata[index.row()][colidx] = newdata

                        break
        else:
            print('No table changes')
        print(self.tabledata[index.row()])
        The_Show.cues.updatecue(index.row(),self.tabledata[index.row()])
        The_Show.cues.savecuelist()

    def setfirstcue(self):
        tblvw = self.findChild(QtWidgets.QTableView)
        tblvw.selectRow(The_Show.cues.currentcueindex)

    def disptext(self):
        self.get_table_data()
        # set the table model
        header = ['Cue Number', 'Act', 'Scene', 'Page', 'ID', 'Title','Dialog/Prompt']
        tablemodel = MyTableModel(self.tabledata, header, self)
        self.tableView.setModel(tablemodel)
        self.tableView.resizeColumnsToContents()

    def get_table_data(self):
        qs = The_Show.cues.cuelist.findall('cue')
        self.tabledata =[]
        for q in qs:
            #print(q.attrib)
            #print(q.find('Move').text)
            self.tabledata.append(
                    [q.find('Move').text,
                    q.find('Act').text,
                    q.find('Scene').text,
                    q.find('Page').text,
                    q.find('Id').text,
                    q.find('Title').text,
                    q.find('Cue').text])
        #print(self.tabledata)

    def openShow(self):
        '''
        Present file dialog to select new ShowConf.xml file
        :return:
        '''
        fdlg = QtWidgets.QFileDialog()
        fname = fdlg.getOpenFileName(self, 'Open file', '/home')
        #fname = QtWidgets.QFileDialog.getOpenFileName(self, 'Open file', '/home')
        fdlg.close()

        print(fname[0])
        The_Show.loadNewShow(fname[0])
        self.setWindowTitle(The_Show.show_conf.settings['name'])
        self.disptext()
        self.setfirstcue()
        self.setWindowTitle(self.show_conf.settings('name'))



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




#The_Show = Show(path.abspath(path.join(path.dirname(__file__))) + '/')
The_Show = Show()
The_Show.displayShow()


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