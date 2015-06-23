##########################################
## Copyright 2012-2013 Ceruti Francesco & contributors
##
## This file is part of LiSP (Linux Show Player).
##########################################

from os import path

absPath = path.abspath(path.join(path.dirname(__file__))) + '/'
print('absPath: ' + absPath)
QLiSPIconsThemePaths = [absPath + 'icons']
QLiSPIconsThemeName = 'lisp'

QLiSPTheme_Dark = ''
with open(absPath + 'theme_dark/style.css', mode='r', encoding='utf-8') as f:
    QLiSPTheme_Dark = f.read().replace('assets/', absPath + 'theme_dark/assets/')
