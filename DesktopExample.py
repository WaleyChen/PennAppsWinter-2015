#!/usr/bin/env python

import argparse
import cv2
from numpy import empty, nan
import os
import sys
import time

import ArbitraryTracking
import numpy as np
import util

ArbitraryTracking = ArbitraryTracking.ArbitraryTracking()

parser = argparse.ArgumentParser(description='Track an object.')
parser.add_argument('inputpath', nargs='?', help='The input path.')
parser.add_argument('--challenge', dest='challenge', action='store_true', help='Enter challenge mode.')
parser.add_argument('--preview', dest='preview', action='store_const', const=True, default=None, help='Force preview')
parser.add_argument('--no-preview', dest='preview', action='store_const', const=False, default=None, help='Disable preview')
parser.add_argument('--no-scale', dest='estimate_scale', action='store_false', help='Disable scale estimation')
parser.add_argument('--with-rotation', dest='estimate_rotation', action='store_true', help='Enable rotation estimation')
parser.add_argument('--bbox', dest='bbox', help='Specify initial bounding box.')
parser.add_argument('--pause', dest='pause', action='store_true', help='Specify initial bounding box.')
parser.add_argument('--output-dir', dest='output', help='Specify a directory for output data.')
parser.add_argument('--quiet', dest='quiet', action='store_true', help='Do not show graphical output (Useful in combination with --output-dir ).')
parser.add_argument('--skip', dest='skip', action='store', default=None, help='Skip the first n frames', type=int)
args = parser.parse_args()

ArbitraryTracking.estimate_scale = args.estimate_scale
ArbitraryTracking.estimate_rotation = args.estimate_rotation
if args.pause:
	pause_time = 0
else:
	pause_time = 10
if args.output is not None:
	if not os.path.exists(args.output):
		os.mkdir(args.output)
	elif not os.path.isdir(args.output):
		raise Exception(args.output + ' exists, but is not a directory')
if args.challenge:
	with open('images.txt') as f:
		images = [line.strip() for line in f]
	init_region = np.genfromtxt('region.txt', delimiter=',')
	num_frames = len(images)
	results = empty((num_frames, 4))
	results[:] = nan
	results[0, :] = init_region
	frame = 0
	im0 = cv2.imread(images[frame])
	im_gray0 = cv2.cvtColor(im0, cv2.COLOR_BGR2GRAY)
	im_draw = np.copy(im0)
	tl, br = (util.array_to_int_tuple(init_region[:2]), util.array_to_int_tuple(init_region[:2] + init_region[2:4])) 
	try: 
		ArbitraryTracking.initialise(im_gray0, tl, br)
		while frame < num_frames:
			im = cv2.imread(images[frame])
			im_gray = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
			ArbitraryTracking.process_frame(im_gray)
			results[frame, :] = ArbitraryTracking.bb
			frame += 1
	except:
		pass

	np.savetxt('output.txt', results, delimiter=',')

else:
	cv2.destroyAllWindows()
	preview = args.preview
	if args.inputpath is not None:
		if os.path.isfile(args.inputpath):
			cap = cv2.VideoCapture(args.inputpath)
			if args.skip is not None:
				cap.set(cv2.cv.CV_CAP_PROP_POS_FRAMES, args.skip)
		else:
			cap = util.FileVideoCapture(args.inputpath)
			if args.skip is not None:
				cap.frame = 1 + args.skip
		if preview is None:
			preview = False
	else:
		cap = cv2.VideoCapture(0)
		if preview is None:
			preview = True
	if not cap.isOpened():
		print 'Unable to open video input.'
		sys.exit(1)
	while preview:
		status, im = cap.read()
		cv2.imshow('Preview', im)
		k = cv2.waitKey(10)
		if not k == -1:
			break
	status, im0 = cap.read()
	im_gray0 = cv2.cvtColor(im0, cv2.COLOR_BGR2GRAY)
	im_draw = np.copy(im0)
	if args.bbox is not None:
		values = args.bbox.split(',')
		try:
			values = [int(v) for v in values]
		except:
			raise Exception('Unable to parse bounding box')
		if len(values) != 4:
			raise Exception('Bounding box must have exactly 4 elements')
		bbox = np.array(values)
		bbox = util.bb2pts(bbox[None, :])
		bbox = bbox[0, :]
		tl = bbox[:2]
		br = bbox[2:4]
	else:
		(tl, br) = util.get_rect(im_draw)
	print 'using', tl, br, 'as init bb'
	ArbitraryTracking.initialise(im_gray0, tl, br)
	frame = 1
	while True:
		status, im = cap.read()
		if not status:
			break
		im_gray = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
		im_draw = np.copy(im)
		tic = time.time()
		ArbitraryTracking.process_frame(im_gray)
		toc = time.time()
		if ArbitraryTracking.has_result:
			cv2.line(im_draw, ArbitraryTracking.tl, ArbitraryTracking.tr, (255, 0, 0), 4)
			cv2.line(im_draw, ArbitraryTracking.tr, ArbitraryTracking.br, (255, 0, 0), 4)
			cv2.line(im_draw, ArbitraryTracking.br, ArbitraryTracking.bl, (255, 0, 0), 4)
			cv2.line(im_draw, ArbitraryTracking.bl, ArbitraryTracking.tl, (255, 0, 0), 4)
		util.draw_keypoints(ArbitraryTracking.tracked_keypoints, im_draw, (255, 255, 255))
		util.draw_keypoints(ArbitraryTracking.votes[:, :2], im_draw)  # blue
		util.draw_keypoints(ArbitraryTracking.outliers[:, :2], im_draw, (0, 0, 255))
		if args.output is not None:
			cv2.imwrite('{0}/input_{1:08d}.png'.format(args.output, frame), im)
			cv2.imwrite('{0}/output_{1:08d}.png'.format(args.output, frame), im_draw)
			with open('{0}/keypoints_{1:08d}.csv'.format(args.output, frame), 'w') as f:
				f.write('x y\n')
				np.savetxt(f, ArbitraryTracking.tracked_keypoints[:, :2], fmt='%.2f')
			with open('{0}/outliers_{1:08d}.csv'.format(args.output, frame), 'w') as f:
				f.write('x y\n')
				np.savetxt(f, ArbitraryTracking.outliers, fmt='%.2f')
			with open('{0}/votes_{1:08d}.csv'.format(args.output, frame), 'w') as f:
				f.write('x y\n')
				np.savetxt(f, ArbitraryTracking.votes, fmt='%.2f')
			with open('{0}/bbox_{1:08d}.csv'.format(args.output, frame), 'w') as f:
				f.write('x y\n')
				np.savetxt(f, np.array((ArbitraryTracking.tl, ArbitraryTracking.tr, ArbitraryTracking.br, ArbitraryTracking.bl, ArbitraryTracking.tl)), fmt='%.2f') 
		if not args.quiet:
			cv2.imshow('main', im_draw)
			k = cv2.waitKey(pause_time)
			key = chr(k & 255)
			if key == 'q':
				break
			if key == 'd':
				import ipdb; ipdb.set_trace()
		im_prev = im_gray
		frame += 1
		print '{5:04d}: center: {0:.2f},{1:.2f} scale: {2:.2f}, active: {3:03d}, {4:04.0f}ms'.format(ArbitraryTracking.center[0], ArbitraryTracking.center[1], ArbitraryTracking.scale_estimate, ArbitraryTracking.active_keypoints.shape[0], 1000 * (toc - tic), frame)
