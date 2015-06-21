'''
Created on Nov 2, 2014
Cue object that maintains the current cue list
Copied to CueEgine project 0n June 6, 2015
    Modified to handle only cues (i.e. no mixer mutes, level, etc.)
    for use with CueEgine
@author: mac
'''
try:
    from lxml import ET
except ImportError:
    import xml.etree.ElementTree as ET


class CueList:
    '''
    CueList object contains information defining the cues for a show
    '''
    def __init__(self, cuefilename):
        '''
        Constructor
        cuelist is a tree object with all cues for the show
        '''
        self.cuelist = ET.parse(cuefilename)
        self.currentcueindex = 0
        self.previewcueindex = 0
        
    def setcurrentcuestate(self, cueindex):
        '''
        Constructor
        '''
        #print('{0:03}'.format(cueindex))
        thiscue = self.cuelist.find("./cue[@num='"+'{0:03}'.format(cueindex)+"']")

        
    def setpreviewcuestate(self, cueindex):
        '''
        Constructor
        '''
        
        