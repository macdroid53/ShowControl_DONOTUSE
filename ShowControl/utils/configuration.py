##########################################
## Copyright 2012-2013 Ceruti Francesco & contributors
##
## This file is part of LiSP (Linux Show Player).
##########################################

#***See: /home/mac/workspace/mylisp/lisp/main.py
#see lines 16-24 how the modules are loaded from folders relative to the module


from configparser import ConfigParser
from shutil import copyfile
import os

HOME = os.path.expanduser("~")

DEFAULT_CFG_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '../default.cfg'))

CFG_DIR = HOME + '/.showcontrol'
CFG_PATH = CFG_DIR + '/config.cfg'


def checkUserConf():
    newcfg = True
    if(not os.path.exists(CFG_PATH)):
        if(not os.path.exists(CFG_DIR)):
            os.makedirs(CFG_DIR)
    else:
        default = ConfigParser()
        default.read(DEFAULT_CFG_PATH)
        current = ConfigParser()
        current.read(CFG_PATH)
        if('Version' in current):
            newcfg = current['Version']['Number'] != default['Version']['Number']
        if(newcfg):
            copyfile(CFG_PATH, CFG_PATH + '.old')
            print('Old configuration file backup -> ' + CFG_PATH + '.old')

    if(newcfg):
        copyfile(DEFAULT_CFG_PATH, CFG_PATH)
        print('Create configuration file -> ' + CFG_PATH)
    else:
        print("Configuration is up to date")

checkUserConf()

config = ConfigParser()
config.read(CFG_PATH)


def toDict():
    conf_dict = {}
    for key in config.keys():
        conf_dict[key] = {}
        for skey in config[key].keys():
            conf_dict[key][skey] = config[key][skey]
    return conf_dict


def updateFromDict(conf):
    for key in conf.keys():
        for skey in conf[key].keys():
            config[key][skey] = conf[key][skey]


def write():
    with open(CFG_PATH, 'w') as cfgfile:
        config.write(cfgfile)
