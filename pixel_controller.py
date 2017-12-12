#!/usr/bin/env python2
# -*- coding: utf-8 -*-
from __future__ import print_function
import os, sys, getopt, time, serial, numpy as np, cv2

"""
Created on Thu Nov 23 15:04:38 2017

@author: hank

Generate data from jpg|png|mp4|avi files and send it to pixelstick

**Attention**:
file head:
    #filename.txt#
    uint32_t
    ......
file end:
    ......
    uint32_t
	-1-1-1-1


Process example:

step 1
img=array([[ 0,  1,  2,  3,  4,  5,  6,  7,  8],
           [ 9, 10, 11, 12, 13, 14, 15, 16, 17],
           [18, 19, 20, 21, 22, 23, 24, 25, 26],
           [27, 28, 29, 30, 31, 32, 33, 34, 35],
           [36, 37, 38, 39, 40, 41, 42, 43, 44],
           [45, 46, 47, 48, 49, 50, 51, 52, 53]])
w = 9
h = 6
n_LEDs = 3
m_LEDs = 3
h_d = 3
v_d = 2

step 2
img=array([[45, 36, 27, 18,  9,  0],
           [46, 37, 28, 19, 10,  1],
           [47, 38, 29, 20, 11,  2],
           [48, 39, 30, 21, 12,  3],
           [49, 40, 31, 22, 13,  4],
           [50, 41, 32, 23, 14,  5],
           [51, 42, 33, 24, 15,  6],
           [52, 43, 34, 25, 16,  7],
           [53, 44, 35, 26, 17,  8]])

step 3
img=array([[45,   , 27,   ,  9,   ],


           [48,   , 30,   , 12,   ],


           [51,   , 33,   , 15,   ],

                                   ])


here each number represent a pixel with RGB info


"""

def HELP():
    print('Usage:\n    -i xxx.jpg -o xxx.cmd -n led_num -r false')
    print('or simply:\n    test.mp4 -n 144\n')
    print('Img or frame will be rotated for 90 degrees clockwisely, thus scaning from left to right')
    print('If you want image to be scaned from up to down, use "-r false"')
    sys.exit()

def find_port():
    port_list = []
    if os.name == 'posix': temp = ['/dev/'+_ for _ in os.listdir('/dev/') if('USB'in _)or('hci'in _)or('rfcomm'in _)]
    else: temp = ['COM'+str(_) for _ in range(32)]
    for _ in temp:
        try:
            s = serial.Serial(_)
            if s.is_open:
                port_list.append(_)
                s.close()
        except:
            continue
    if port_list:
        if len(port_list)==1: return port_list[0]
        print('choose port from %s: '%' | '.join(port_list))
        while 1:
            _ = raw_input('name: ')
            if _ in port_list:
                return _
            print('invalid input...')
    raise IOError('No port available! Abort.')

def get_Step(s):
    try:
        print('connected. shaking hands')
        s.write('time')
        time.sleep(2.5)
        _ = s.read_all()
        _ = _[_.find('t_Step')+7:]
        t_Step = int(_[:_.find('\r\n')])/1000.
    except:
        t_Step = 0.03
    return t_Step

def parse_opt():
    n_LEDs = 30
    rotate = True
    try:
        if len(sys.argv) > 1:
            opts, rst = getopt.getopt(sys.argv[1:], 'ho:i:n:r:')
        else: HELP()
    except getopt.GetoptError, e:
        print("Unknown parameter '{}'".format(e.opt))
        HELP()

    for i,j in opts:
        if i == '-h': HELP()
        elif i == '-i':
            if j[-3:] in ['jpg', 'png', 'mp4', 'avi']:
                from_file = j
            else:
                print(j + ': unsupport type image.')
                HELP()
        elif i == '-o':
            if not j.endswith('cmd') and '.' in j:
                to_file = j[:j.index('.')]
            else: to_file = j
        elif i == '-n':
            n_LEDs = int(j)
        elif i == '-r' and j == 'false':
            rotate = False
    try:
        if '-i' not in opts:
            from_file = sys.argv[1]
        if '-o' not in opts:
            to_file = from_file[:-4]
    except: HELP()
    return from_file, to_file, n_LEDs, rotate



if __name__ == '__main__':
    # this is virtual leds that in the perpendicular direction of pixelstick
    # in other word, in the parallel direction with movement
    from_file, to_file, n_LEDs, rotate = parse_opt()
    m_LEDs = 100

    cmd_Step = 0.0015 # 1.5ms
    extra_Step = 0.015 # 15ms

    port = find_port()
    print('Using port: '+port)

    s = serial.Serial(port = port, baudrate = 115200)
    t_Step = get_Step(s)
    print('Current s_Step: {}ms'.format(t_Step*1000))

    if from_file[-3:] in ['jpg', 'png']:
        MODE = 'PHOTO'
        # step 1
        img = cv2.imread(from_file)
        if rotate:
            img = cv2.rotate(img, cv2.ROTATE_90_CLOCKWISE)

    elif from_file[-3:] in ['mp4', 'avi']:
        MODE = 'VIDEO'
        # open video file
        video = cv2.VideoCapture(from_file)
        fps = video.get(cv2.CAP_PROP_FPS)
        # frame_time_duration
        ftd = 1./fps
        # define an anonymous function to return next frame
        # read() --> [bool, img]
        new = lambda b=rotate, c=video: cv2.rotate(c.read()[1], 2) if b else c.read()[1]
        try: img = new()
        except: HELP()
        if img is None: HELP()


    # step 2
    h, w = img.shape[:2]
    v_d = w/n_LEDs
    h_d = h/m_LEDs
    # step 3
    img = img[::h_d, ::v_d]

    # save cmd file to debug, you can comment it
    to_file += '_{}x{}_n{}'.format(w, h, n_LEDs) + '.cmd'
    f = open(to_file, 'w')

    # awake pixelstick to send data
    s.write('push')
    time.sleep(3)
    print(s.read_all(), end='')

    f.write('#%s#\n'%to_file)
    s.write('#%s#\n'%to_file)

    cv2.namedWindow('src', cv2.WINDOW_FREERATIO)
    stamp = time.time()
    while 1:
        try:
            for row in range(img.shape[0]):
                cv2.imshow('src', cv2.line(img.copy(),
                                           (0, row),
                                           (img.shape[1], row),
                                           (10, 240, 5),
                                           2, cv2.LINE_AA))
                for i, (b, g, r) in enumerate(img[row]): # opencv is BGR mode
                    # filter repeating datas
                    if row > 0:
                        if ([b, g, r] == img[row-1, i]).all():
                            continue

                    # adjust cmd_Step for best performance
                    time.sleep(cmd_Step)
                    print(s.read_all(), end='')

                    t = str(i<<24|r<<16|g<<8|b) + '\n' # pixelstick is GRB mode, but we can set it in arduino
                    f.write(t)
                    s.write(t)

                # adjust extra_Step for best performance
                time.sleep(t_Step + extra_Step)
                print(s.read_all(), end='')
                if t != '\n':
                    t = '\n'
                    f.write(t)
                    s.write(t)

                if MODE == 'VIDEO':
                    duration = time.time() - stamp
                    frames = int(duration/ftd)
                    for _ in range(frames): img = new()
                    if img is None: break # going to the end of the video
                    img = img[::h_d, ::v_d]
                    stamp += ftd * frames

            if MODE == 'PHOTO': break

        except KeyboardInterrupt:
            f.write('-1-1-1-1')
            s.write('-1-1-1-1')
            print('Terminated.')
            sys.exit()

    print('Done!')
    print('Image size: %d x %d, v_d: %d, h_d: %d'%(w, h, v_d, h_d))
    print('n_LEDs: %d  m_LEDs: %d'%(n_LEDs, m_LEDs))
    print('From %s to %s'%(from_file, to_file))
    print(s.read_all())
    f.write('-1-1-1-1')
    s.write('-1-1-1-1')
    time.sleep(8)
    print(s.read_all())
    f.close()
    s.close()
    if MODE == 'VIDEO': video.release()
