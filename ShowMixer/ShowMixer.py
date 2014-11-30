#!/usr/bin/env python3
import sys
import types
import argparse

from PyQt5 import Qt, QtCore, QtGui, QtWidgets
import xml.etree.ElementTree as ET
from os import path

from ShowConf import ShowConf
from MixerConf import MixerConf
from MixerMap import MixerCharMap
from Cues import CueList

#import ui_DynChanStripDlg
import mainwindow
from pythonosc import osc_message_builder
from pythonosc import udp_client

from lisp.ui import styles

def translate(value, leftMin, leftMax, rightMin, rightMax):
    # Figure out how 'wide' each range is
    leftSpan = leftMax - leftMin
    rightSpan = rightMax - rightMin

    # Convert the left range into a 0-1 range (float)
    valueScaled = float(value - leftMin) / float(leftSpan)

    # Convert the 0-1 range into a value in the right range.
    return rightMin + (valueScaled * rightSpan)



class Show:
    '''
    The Show class contains the information and object that constitute a show
    '''


    def __init__(self, show_confpath):
        '''
        Constructor
        '''
        self.show_conf = ShowConf(show_confpath + 'ShowConf.xml')
        self.mixer = MixerConf(show_confpath + 'MixerDefs-r2.xml',self.show_conf.settings['mxrmfr'],self.show_conf.settings['mxrmodel'])
        self.chrchnmap = MixerCharMap(show_confpath + 'MixerMap.xml')
        self.cues = CueList(show_confpath + 'Scrooge Moves.xml', self.mixer.input_count)
        self.cues.currentcueindex = 0
        self.cues.setcurrentcuestate(self.cues.currentcueindex)


class ChanStripDlg(QtWidgets.QMainWindow, mainwindow.Ui_MainWindow):
    ChanStrip_MinWidth = 50

    def __init__(self, cuelistfile, parent=None):
        super(ChanStripDlg, self).__init__(parent)
        QtGui.QIcon.setThemeSearchPaths(styles.QLiSPIconsThemePaths)
        QtGui.QIcon.setThemeName(styles.QLiSPIconsThemeName)
        self.__index = 0
        self.setupUi(self)
        self.nextButton.clicked.connect(self.on_buttonNext_clicked)

    def addChanStrip(self):
        layout = self.stripgridLayout
        self.channumlabels = []
        self.mutes = []
        self.levs = []
        self.sliders = []
        self.scrbls = []
        for i in range(1,chans+1):
            #print(str(i))
            #Add scribble for this channel Qt::AlignHCenter
            scrbl = QtWidgets.QLabel()
            scrbl.setObjectName('scr' + '{0:02}'.format(i))
            scrbl.setText('Scribble ' + '{0:02}'.format(i))
            scrbl.setAlignment(QtCore.Qt.AlignHCenter)
            scrbl.setMinimumWidth(self.ChanStrip_MinWidth)
            scrbl.setMinimumHeight(30)
            scrbl.setWordWrap(True)
            layout.addWidget(scrbl,4,i-1,1,1)
            self.scrbls.append(scrbl)

            #Add slider for this channel
            sldr = QtWidgets.QSlider(QtCore.Qt.Vertical)
            sldr.valueChanged.connect(self.sliderprint)
            sldr.setObjectName('{0:02}'.format(i))
            sldr.setMinimumSize(self.ChanStrip_MinWidth,200)
            sldr.setRange(0,1024)
            sldr.setTickPosition(3)
            sldr.setTickInterval(10)
            sldr.setSingleStep(2)
            layout.addWidget(sldr,3,i-1,1,1)
            self.sliders.append(sldr)

            #Add label for this channel level
            lev = QtWidgets.QLabel()
            lev.setObjectName('lev' + '{0:02}'.format(i))
            lev.setText('000')
            lev.setMinimumWidth(self.ChanStrip_MinWidth)
            lev.setAlignment(QtCore.Qt.AlignHCenter)
            layout.addWidget(lev,2,i-1,1,1)
            self.levs.append(lev)

            #Add mute button for this channel
            mute = QtWidgets.QPushButton()
            mute.setCheckable(True)            
            mute.clicked.connect(self.on_buttonMute_clicked)
            mute.setObjectName('{0:02}'.format(i))
            mute.setMinimumWidth(self.ChanStrip_MinWidth)
            layout.addWidget(mute,1,i-1,1,1)
            self.mutes.append(mute)

            #Add label for this channel
            lbl = QtWidgets.QLabel()
            lbl.setObjectName('{0:02}'.format(i))
            lbl.setText('Ch' + '{0:02}'.format(i))
            lbl.setMinimumWidth(self.ChanStrip_MinWidth)
            layout.addWidget(lbl,0,i-1,1,1)
            self.channumlabels.append(lbl)
        #self.setLayout(layout)

    def sliderprint(self, val):
        sending_slider = self.sender()
        print(sending_slider.objectName())

        scrLbl = self.findChild(QtWidgets.QLabel, name='lev' + sending_slider.objectName())
        print(scrLbl.text())
        scrLbl.setText('{0:03}'.format(val))
        print(val)
        osc_add='/ch/' + sending_slider.objectName() + '/mix/fader'
        msg = osc_message_builder.OscMessageBuilder(address=osc_add)
        msg.add_arg(translate(val, 0,1024,0.0, 1.0))
        msg = msg.build()
        client.send(msg)

    def on_buttonNext_clicked(self):
#         print(The_Show.cues.mutestate)
#         print('Next')
        previdx = The_Show.cues.currentcueindex
        The_Show.cues.currentcueindex += 1
        tblvw = self.findChild(QtWidgets.QTableView)
        tblvw.selectRow(The_Show.cues.currentcueindex)
        #print('Old index: ' + str(previdx) + '   New: ' + str(The_Show.cues.currentcueindex))
        The_Show.cues.setcurrentcuestate(The_Show.cues.currentcueindex)
        #print(The_Show.cues.mutestate)
        
        for btncnt in range(1, The_Show.mixer.inputsliders.__len__() + 1):
            mute = self.findChild(QtWidgets.QPushButton, name='{0:02}'.format(btncnt))
#             print('Object name: ' + mute.objectName())
#             print('ch' + '{0}'.format(btncnt))
#             print(The_Show.cues.mutestate['ch7'])
#             print(The_Show.cues.mutestate['ch' + '{0}'.format(btncnt)])
            osc_add='/ch/' + mute.objectName() + '/mix/on'
            #print(osc_add)
            msg = osc_message_builder.OscMessageBuilder(address=osc_add)
            if The_Show.cues.mutestate['ch' + '{0}'.format(btncnt)] == 1:
                mute.setChecked(False)
                msg.add_arg(The_Show.mixer.mutestyle['unmute'])
            else:
                mute.setChecked(True)
                msg.add_arg(The_Show.mixer.mutestyle['mute'])
            #print(mute.objectName())
            msg = msg.build()
            client.send(msg)

    def initmutes(self):
        
        for btncnt in range(1, The_Show.mixer.inputsliders.__len__() + 1):
            mute = self.findChild(QtWidgets.QPushButton, name='{0:02}'.format(btncnt))
#             print('Object name: ' + mute.objectName())
#             print('ch' + '{0}'.format(btncnt))
#             print(The_Show.cues.mutestate['ch7'])
#             print(The_Show.cues.mutestate['ch' + '{0}'.format(btncnt)])
            osc_add='/ch/' + mute.objectName() + '/mix/on'
            #print(osc_add)
            msg = osc_message_builder.OscMessageBuilder(address=osc_add)
            if The_Show.cues.mutestate['ch' + '{0}'.format(btncnt)] == 1:
                mute.setChecked(False)
                msg.add_arg(The_Show.mixer.mutestyle['unmute'])
            else:
                mute.setChecked(True)
                msg.add_arg(The_Show.mixer.mutestyle['mute'])
            #print(mute.objectName())
            msg = msg.build()
            client.send(msg)


    def on_buttonMute_clicked(self):
        mbtn=self.sender()
        print(mbtn.objectName())
        chkd = mbtn.isChecked()
        dwn=mbtn.isDown()
        osc_add='/ch/' + mbtn.objectName() + '/mix/on'
        print(osc_add)
        print(chkd)
        print(dwn)
        msg = osc_message_builder.OscMessageBuilder(address=osc_add)
        if mbtn.isChecked():
            msg.add_arg(The_Show.mixer.mutestyle['mute'])
        else:
            msg.add_arg(The_Show.mixer.mutestyle['unmute'])

        msg = msg.build()
        client.send(msg)


    def on_buttonUnmute_clicked(self):
        spin=1
        chnum=self.spinChNum.value()
        print(chnum)
        print(spin)
        osc_add='/ch/' + '{0:02}'.format(chnum) + '/mix/on'
#        osc_add='/ch/02/mix/on'
        msg = osc_message_builder.OscMessageBuilder(address=osc_add)
        msg.add_arg(1)
        msg = msg.build()
        client.send(msg)

    def set_scribble(self, mxrmap):
        chans = mxrmap.findall('input')
        for chan in chans:
            cnum = int(chan.attrib['chan'])
            osc_add='/ch/' + '{0:02}'.format(cnum) + '/config/name'
            msg = osc_message_builder.OscMessageBuilder(address=osc_add)
            tmpstr = chan.attrib['actor'][:5]
            #print('Temp String: ' + tmpstr)
            msg.add_arg(chan.attrib['actor'][:5])
            msg = msg.build()
            client.send(msg)
            thislbl = self.findChild(QtWidgets.QLabel, name='scr'+ '{0:02}'.format(cnum))
            thislbl.setText(tmpstr)

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
#print('Show Object:',The_Show)
#print(The_Show.show_conf.settings['mxrmapfile'])
insliders = The_Show.mixer.inputsliders
#print('Input Slider Count: ' + str(len(insliders)))
for x in range(1, len(insliders)+1):
    sliderconf = insliders['Ch' + '{0:02}'.format(x)]
    #print('level: ' + str(sliderconf.level))
    #print('scribble: ' + sliderconf.scribble_text)
outsliders = The_Show.mixer.outputsliders
#print('Output Slider Count: ' + str(len(outsliders)))
for x in range(1, len(outsliders)+1):
    sliderconf = outsliders['Ch' + '{0:02}'.format(x)]
    #print('level: ' + str(sliderconf.level))
    #print('scribble: ' + sliderconf.scribble_text)
    
#print(The_Show.cues)
qs = The_Show.cues.cuelist.findall('cue')
# for q in qs:
#     print(q.attrib)

#print(The_Show.chrchnmap)
chs = The_Show.chrchnmap.maplist.findall('input')
# for ch in chs:
#     print(ch.attrib)

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
#     app.setStyleSheet(""" QPushButton {color: blue;
#                          background-color: yellow;
#                          selection-color: blue;
#                          selection-background-color: green;}""")
    #app.setStyleSheet("QPushButton {pressed-color: red }")
    app.setStyleSheet(styles.QLiSPTheme_Dark)
    chans = len(The_Show.mixer.inputsliders)
    ui = ChanStripDlg(path.abspath(path.join(path.dirname(__file__))) + '/Scrooge Moves.xml')
    ui.resize(chans*ui.ChanStrip_MinWidth,800)
    ui.addChanStrip()
    ui.disptext()
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", default="192.168.53.40", help="The ip of the OSC server")
    parser.add_argument("--port", type=int, default=10023, help="The port the OSC server is listening on")
    args = parser.parse_args()

    client = udp_client.UDPClient(args.ip, args.port)
    ui.set_scribble(The_Show.chrchnmap.maplist)
    ui.initmutes()
    tblvw = ui.findChild(QtWidgets.QTableView)
    tblvw.selectRow(The_Show.cues.currentcueindex)

    ui.show()
    sys.exit(app.exec_())