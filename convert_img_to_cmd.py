#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Thu Nov 23 15:04:38 2017

@author: hank

Generate data from jpg|png files for usage of pixelstick

**Attention**:
file head:
	#filename.txt#
	i, r, g, b
file end:
	i, r, g, b
	#255#255#
"""

import sys, getopt
from PIL import Image

try:
    opts, _ = getopt.getopt(sys.argv[1:], 'ho:i:n:', ['help','input=','output=','num='])
except getopt.GetoptError, e:
    print("Unknown parameter '{}'".format(e.opt))
    quit

n_LEDs = 30
v_rate = 5

for i,j in opts:
    if i in ['-h', '--help']:
        print('Usage:\n\t-i xxx.jpg -o xxx.txt\nor:\n\t--input xxx.png --output xxx')
        quit
    elif i in ['-i', '--input']:
        if j[-3:] in ['jpg', 'png']:
            In = j
        else:
            print(j + ': unsupport type image.')
            quit
    elif i in ['-o', '--output']:
        if not j.endswith('txt'):
            if '.' in j:
                Out = j[:j.index('.')] + '.txt'
            else:
                Out = j + '.txt'
        else: Out = j
    elif i in ['-n', '--num']:
        if isinstance(j, int):
            n_LEDs = j



img = Image.open(In)
w, h = img.size# Width x Height
img = list(img.rotate(-90, expand=1).getdata())# a Height x Width 1-D array

# What if there are numpy and cv2 on every computers.
# Then I wont need to be disgusted by PIL.Image
v_duration = h/n_LEDs
h_duration = w/(v_rate*n_LEDs)

with open(Out, 'w') as f:
	# head indicator
    f.write('#%s#\n\n'%Out)
    f.write('  i   r   g   b\n')
    for j in range(v_rate*n_LEDs):
        for i in range(n_LEDs):
            r, g, b = img[j * h_duration * h + i * v_duration]
            f.write('%3d,%3d,%3d,%3d\n'%(i, r, g, b))
        f.write('\n')
    # end indicator
    f.write('#255#255#')

print('Done!')
print('Image size: %4d x %4d, v_duration: %d, h_duration: %d'%(h, w, v_duration, h_duration))
print(i, j)
