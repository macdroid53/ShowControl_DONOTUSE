'''
Created on Nov 2, 2014

@author: mac
'''
try:
    from lxml import ET
except ImportError:
    import xml.etree.ElementTree as ET


class MixerCharMap:
    '''
    classdocs
    '''


    def __init__(self, mapfilename):
        '''
        Constructor
        '''
        self.maplist = ET.parse(mapfilename)
        