import cv2
import itertools
from numpy import array, zeros, vstack, hstack, math, nan, argsort, median, \
	argmax, isnan, append
import scipy.cluster
import scipy.spatial
import time
import numpy as np
import util

class ArbitraryTracking(object):
	DETECTOR = 'BRISK'
	DESCRIPTOR = 'BRISK'
	DESC_LENGTH = 512
	MATCHER = 'BruteForce-Hamming'
	THR_OUTLIER = 20
	THR_CONF = 0.75
	THR_RATIO = 0.8
	estimate_scale = True
	estimate_rotation = True
	def initialise(self, im_gray0, tl, br):
		self.detector = cv2.FeatureDetector_create(self.DETECTOR)
		self.descriptor = cv2.DescriptorExtractor_create(self.DESCRIPTOR)
		self.matcher = cv2.DescriptorMatcher_create(self.MATCHER)
		keypoints_cv = self.detector.detect(im_gray0)
		ind = util.in_rect(keypoints_cv, tl, br)
		selected_keypoints_cv = list(itertools.compress(keypoints_cv, ind))
		selected_keypoints_cv, self.selected_features = self.descriptor.compute(im_gray0, selected_keypoints_cv)
		selected_keypoints = util.keypoints_cv_to_np(selected_keypoints_cv)
		num_selected_keypoints = len(selected_keypoints_cv)
		if num_selected_keypoints == 0:
			raise Exception('No keypoints found in selection')
		background_keypoints_cv = list(itertools.compress(keypoints_cv, ~ind))
		background_keypoints_cv, background_features = self.descriptor.compute(im_gray0, background_keypoints_cv)
		_ = util.keypoints_cv_to_np(background_keypoints_cv)
		self.selected_classes = array(range(num_selected_keypoints)) + 1
		background_classes = zeros(len(background_keypoints_cv))
		self.features_database = vstack((background_features, self.selected_features))
		self.database_classes = hstack((background_classes, self.selected_classes))
		pdist = scipy.spatial.distance.pdist(selected_keypoints)
		self.squareform = scipy.spatial.distance.squareform(pdist)
		angles = np.empty((num_selected_keypoints, num_selected_keypoints))
		for k1, i1 in zip(selected_keypoints, range(num_selected_keypoints)):
			for k2, i2 in zip(selected_keypoints, range(num_selected_keypoints)):
				v = k2 - k1
				angle = math.atan2(v[1], v[0])
				angles[i1, i2] = angle
		self.angles = angles
		center = np.mean(selected_keypoints, axis=0)
		self.center_to_tl = np.array(tl) - center
		self.center_to_tr = np.array([br[0], tl[1]]) - center
		self.center_to_br = np.array(br) - center
		self.center_to_bl = np.array([tl[0], br[1]]) - center
		self.springs = selected_keypoints - center
		self.im_prev = im_gray0
		self.active_keypoints = np.copy(selected_keypoints)
		self.active_keypoints = hstack((selected_keypoints, self.selected_classes[:, None]))
		self.num_initial_keypoints = len(selected_keypoints_cv)
	def estimate(self, keypoints):
		center = array((nan, nan))
		scale_estimate = nan
		med_rot = nan
		if keypoints.size > 1:
			keypoint_classes = keypoints[:, 2].squeeze().astype(np.int) 
			if keypoint_classes.size == 1:
				keypoint_classes = keypoint_classes[None]
			ind_sort = argsort(keypoint_classes)
			keypoints = keypoints[ind_sort]
			keypoint_classes = keypoint_classes[ind_sort]
			all_combs = array([val for val in itertools.product(range(keypoints.shape[0]), repeat=2)])	
			all_combs = all_combs[all_combs[:, 0] != all_combs[:, 1], :]
			ind1 = all_combs[:, 0] 
			ind2 = all_combs[:, 1]
			class_ind1 = keypoint_classes[ind1] - 1
			class_ind2 = keypoint_classes[ind2] - 1
			duplicate_classes = class_ind1 == class_ind2
			if not all(duplicate_classes):
				ind1 = ind1[~duplicate_classes]
				ind2 = ind2[~duplicate_classes]
				class_ind1 = class_ind1[~duplicate_classes]
				class_ind2 = class_ind2[~duplicate_classes]
				pts_allcombs0 = keypoints[ind1, :2]
				pts_allcombs1 = keypoints[ind2, :2]
				dists = util.L2norm(pts_allcombs0 - pts_allcombs1)
				original_dists = self.squareform[class_ind1, class_ind2]
				scalechange = dists / original_dists
				angles = np.empty((pts_allcombs0.shape[0]))
				v = pts_allcombs1 - pts_allcombs0
				angles = np.arctan2(v[:, 1], v[:, 0])
				original_angles = self.angles[class_ind1, class_ind2]
				angle_diffs = angles - original_angles
				long_way_angles = np.abs(angle_diffs) > math.pi
				angle_diffs[long_way_angles] = angle_diffs[long_way_angles] - np.sign(angle_diffs[long_way_angles]) * 2 * math.pi
				scale_estimate = median(scalechange)
				if not self.estimate_scale:
					scale_estimate = 1;
				med_rot = median(angle_diffs)
				if not self.estimate_rotation:
					med_rot = 0;
				keypoint_class = keypoints[:, 2].astype(np.int)
				votes = keypoints[:, :2] - scale_estimate * (util.rotate(self.springs[keypoint_class - 1], med_rot))
				self.votes = votes
				pdist = scipy.spatial.distance.pdist(votes)
				linkage = scipy.cluster.hierarchy.linkage(pdist)
				T = scipy.cluster.hierarchy.fcluster(linkage, self.THR_OUTLIER, criterion='distance')
				cnt = np.bincount(T)
				Cmax = argmax(cnt)
				inliers = T == Cmax
				self.outliers = keypoints[~inliers, :]
				keypoints = keypoints[inliers, :]
				votes = votes[inliers, :]
				center = np.mean(votes, axis=0)
		return (center, scale_estimate, med_rot, keypoints)
	def process_frame(self, im_gray):
		tracked_keypoints, _ = util.track(self.im_prev, im_gray, self.active_keypoints)
		(center, scale_estimate, rotation_estimate, tracked_keypoints) = self.estimate(tracked_keypoints)
		keypoints_cv = self.detector.detect(im_gray) 
		keypoints_cv, features = self.descriptor.compute(im_gray, keypoints_cv)
		active_keypoints = zeros((0, 3)) 
		matches_all = self.matcher.knnMatch(features, self.features_database, 2)
		if not any(isnan(center)):
			selected_matches_all = self.matcher.knnMatch(features, self.selected_features, len(self.selected_features))
		if len(keypoints_cv) > 0:
			transformed_springs = scale_estimate * util.rotate(self.springs, -rotation_estimate)
			for i in range(len(keypoints_cv)):
				location = np.array(keypoints_cv[i].pt)
				matches = matches_all[i]
				distances = np.array([m.distance for m in matches])
				combined = 1 - distances / self.DESC_LENGTH
				classes = self.database_classes
				bestInd = matches[0].trainIdx
				secondBestInd = matches[1].trainIdx
				ratio = (1 - combined[0]) / (1 - combined[1])
				keypoint_class = classes[bestInd]
				if ratio < self.THR_RATIO and combined[0] > self.THR_CONF and keypoint_class != 0:
					new_kpt = append(location, keypoint_class)
					active_keypoints = append(active_keypoints, array([new_kpt]), axis=0)
				if not any(isnan(center)):
					matches = selected_matches_all[i]				
					distances = np.array([m.distance for m in matches])
					idxs = np.argsort(np.array([m.trainIdx for m in matches]))
					distances = distances[idxs]					
					confidences = 1 - distances / self.DESC_LENGTH
					relative_location = location - center
					displacements = util.L2norm(transformed_springs - relative_location)
					weight = displacements < self.THR_OUTLIER  # Could be smooth function
					combined = weight * confidences
					classes = self.selected_classes
					sorted_conf = argsort(combined)[::-1]  # reverse
					bestInd = sorted_conf[0]
					secondBestInd = sorted_conf[1]
					ratio = (1 - combined[bestInd]) / (1 - combined[secondBestInd])
					keypoint_class = classes[bestInd]
					if ratio < self.THR_RATIO and combined[bestInd] > self.THR_CONF and keypoint_class != 0:
						new_kpt = append(location, keypoint_class)
						if active_keypoints.size > 0:
							same_class = np.nonzero(active_keypoints[:, 2] == keypoint_class)
							active_keypoints = np.delete(active_keypoints, same_class, axis=0)
						active_keypoints = append(active_keypoints, array([new_kpt]), axis=0)
		if tracked_keypoints.size > 0:
			tracked_classes = tracked_keypoints[:, 2]
			if active_keypoints.size > 0:
				associated_classes = active_keypoints[:, 2]
				missing = ~np.in1d(tracked_classes, associated_classes)
				active_keypoints = append(active_keypoints, tracked_keypoints[missing, :], axis=0)
			else:
				active_keypoints = tracked_keypoints
		_ = active_keypoints
		self.center = center
		self.scale_estimate = scale_estimate
		self.rotation_estimate = rotation_estimate
		self.tracked_keypoints = tracked_keypoints
		self.active_keypoints = active_keypoints
		self.im_prev = im_gray
		self.keypoints_cv = keypoints_cv
		_ = time.time()
		self.tl = (nan, nan)
		self.tr = (nan, nan)
		self.br = (nan, nan)
		self.bl = (nan, nan)
		self.bb = array([nan, nan, nan, nan])
		self.has_result = False
		if not any(isnan(self.center)) and self.active_keypoints.shape[0] > self.num_initial_keypoints / 10:
			self.has_result = True
			tl = util.array_to_int_tuple(center + scale_estimate * util.rotate(self.center_to_tl[None, :], rotation_estimate).squeeze())
			tr = util.array_to_int_tuple(center + scale_estimate * util.rotate(self.center_to_tr[None, :], rotation_estimate).squeeze())
			br = util.array_to_int_tuple(center + scale_estimate * util.rotate(self.center_to_br[None, :], rotation_estimate).squeeze())
			bl = util.array_to_int_tuple(center + scale_estimate * util.rotate(self.center_to_bl[None, :], rotation_estimate).squeeze())
			min_x = min((tl[0], tr[0], br[0], bl[0]))
			min_y = min((tl[1], tr[1], br[1], bl[1]))
			max_x = max((tl[0], tr[0], br[0], bl[0]))
			max_y = max((tl[1], tr[1], br[1], bl[1]))
			self.tl = tl
			self.tr = tr
			self.bl = bl
			self.br = br
			self.bb = np.array([min_x, min_y, max_x - min_x, max_y - min_y])
