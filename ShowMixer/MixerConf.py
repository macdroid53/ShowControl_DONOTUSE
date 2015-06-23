#!/usr/bin/env python3
'''
Created on Oct 19, 2014
MixerConf mixer configuration object

@author: mac
'''
try:
    from lxml import ET
except ImportError:
    import xml.etree.ElementTree as ET

class InputControl:
    def __init__(self, level, scribble_text):
        self.level = level
        self.scribble_text = scribble_text

class OutputControl:
    def __init__(self, level, scribble_text):
        self.level = level
        self.scribble_text = scribble_text

class MixerConf:
    '''
    Created on Oct 19, 2014
    MixerConf object returns the configuration of the mixer specified
    by the mixername and mixermodel arguments.
    
    It searches the file specified in mixerconf_file for the mixer specified
    by the mixername and mixermodel arguments.  
    
    MixerConf structure:
    MixerConf
        protocol (string)
        inputsliders {dictionary} containing cnt keys, where each key:
            key : InputControl (object)
        outputsliders {dictionary} containing cnt keys, where each key:
            key : OutputControl (object)
    @author: mac
    '''
    def __init__(self, mixerconf_file, mixername, mixermodel):
        #
        # dictionary of input sliders, index format: [Chnn]
        # each entry is a InputControl object
        self.inputsliders = {}

        # dictionary of output sliders, index format: [Chnn]
        # each entry is a OutputControl object
        self.outputsliders = {}

        # dictionary of mutestyle for the mixer
        # mutestyle referes to how the mixer indicates the channel is muted
        # for example, the Yamaha 01V indicates a channel is un-muted with an illuminated light
        # other mixer indicate a muted channel with an illuminated light
        # mutestyle['mutestyle'] will be the string 'illuminated' or 'non-illumnated'
        #                        as read from <mixerdefs>.xml for each particular mixer
        # for mutestyle['illuminated'], mutestyle['mute'] will be 0, mutestyle['unmute'] will be 1
        # for mutestyle['non-illuminated'], mutestyle['mute'] will be 1, mutestyle['unmute'] will be 0
        self.mutestyle = {}

        mixerdefs = ET.parse(mixerconf_file)
        mixers = mixerdefs.getroot()
        #print('mixers: ' + str(mixers))
        for mixer in mixers:
            #print(mixer.attrib)
            mxattribs = mixer.attrib
            if 'model' in mxattribs.keys():
                if mxattribs['model'] == mixermodel and mxattribs['mfr'] == mixername:
                    #print('found')
                    break
        self.protocol = mixer.find('protocol').text
        #print('protocol: ' + self.protocol)
        self.mutestyle['mutestyle'] = mixer.find('mutestyle').text
        if self.mutestyle['mutestyle'] == 'illuminated':
            self.mutestyle['mute'] = 0
            self.mutestyle['unmute'] = 1
        else:
            self.mutestyle['mute'] = 1
            self.mutestyle['unmute'] = 0
        ports = mixer.findall('port')
        for port in ports:
            portattribs = port.attrib
            #print(portattribs)
            if portattribs['type'] == 'input':
                #print(portattribs['cnt'])
                self.input_count = int(portattribs['cnt'])
                #print(self.input_count)
                for x in  range(1, self.input_count + 1):
                    sldr = InputControl(x,'In' + '{0:02}'.format(x))
                    self.inputsliders['Ch' + '{0:02}'.format(x)] = sldr
            elif portattribs['type'] == 'output':
                #print(portattribs['cnt'])
                self.output_count = int(portattribs['cnt'])
                #print(self.output_count)
                for x in  range(1, self.output_count + 1):
                    sldr = OutputControl(x,'Out' + '{0:02}'.format(x))
                    self.outputsliders['Ch' + '{0:02}'.format(x)] = sldr 
                    
        #print(self.inputsliders)