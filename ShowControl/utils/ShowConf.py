#!/usr/bin/env python3
'''
Created on Oct 19, 2014
Show configuration object
contains information defining the show
@author: mac
'''

try:
    from lxml import ET
except ImportError:
    import xml.etree.ElementTree as ET


class ShowConf:
    '''
    Created on Oct 19, 2014
    ShowConf object contains information defining the show
    returns a dictionary containing the settings defining a show
    reads settings from file showconf_file
    keys in ShowConf dictionary:
        name       : <name of the show>
        mxrmfr     : <mixer manufacturer>
        mxrmodel   : <mixer model>
        mxrmapfile : <xml file containing channel to actor map>
        mxrcuefile : <xml file containing mixer cues for the show>
    @author: mac
    '''
    def __init__(self, showconf_file):
        self.settings = {}
        tree = ET.parse(showconf_file)
        doc = tree.getroot()
        print(doc)

#Get mixer info
        mixer = doc.find('mixer')
        print('ShowConf::', mixer.attrib)
        mxattribs = mixer.attrib
        try:
            print(mxattribs['model'])
            self.settings['mxrmodel'] = mxattribs['model'] 
        except:
            self.settings['mxrmodel'] = ''
            print('No Mixer model defined')
        if self.settings['mxrmodel'] == '':
            self.settings['mxrmodel'] = ''
        try:
            print(mxattribs['mfr'])
            self.settings['mxrmfr'] = mxattribs['mfr']
        except:
            self.settings['mxrmfr'] = 'Default'
            print('No Mixer manufacturer defined')
        if self.settings['mxrmfr'] == '':
            self.settings['mxrmfr'] = 'Default'

        #Get mixer chan to actor/char map file name
        mxrmap = doc.find('mixermap')
        attribs = mxrmap.attrib
        self.settings["mxrmap"] = attribs['file']

        #Get mixer chan to actor/char map file name
        mxrcues = doc.find('mixercues')
        attribs = mxrcues.attrib
        self.settings["mxrcue"] = attribs['file']
        
        print(self.settings)
        self.name = doc.find('name')
        print('ShowConf.__init__ name: ',self.name.text)
        self.settings['name'] = self.name.text
