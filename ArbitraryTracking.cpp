#include "ArbitraryTracking.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>

#if __cplusplus < 201103L
    #include <limits>
    #ifndef NAN
        #define NAN std::numeric_limits<float>::quiet_NaN()
        template <T>
        bool isnan(T d) {
          return d != d;
        }
    #endif
#endif

void inout_rect(const std::vector<cv::KeyPoint>& keypoints, cv::Point2f topleft, cv::Point2f bottomright, std::vector<cv::KeyPoint>& in, std::vector<cv::KeyPoint>& out) {
    for(unsigned int i = 0; i < keypoints.size(); i++) {
        if(keypoints[i].pt.x > topleft.x && keypoints[i].pt.y > topleft.y && keypoints[i].pt.x < bottomright.x && keypoints[i].pt.y < bottomright.y)
            in.push_back(keypoints[i]);
        else out.push_back(keypoints[i]);
    }
}


void track(cv::Mat im_prev, cv::Mat im_gray, const std::vector<std::pair<cv::KeyPoint, int> >& keypointsIN, std::vector<std::pair<cv::KeyPoint, int> >& keypointsTracked, std::vector<unsigned char>& status, int THR_FB) {
    status = std::vector<unsigned char>();
    if(keypointsIN.size() > 0) {
        std::vector<cv::Point2f> pts;
        std::vector<cv::Point2f> pts_back;
        std::vector<cv::Point2f> nextPts;
        std::vector<unsigned char> status_back;
        std::vector<float> err;
        std::vector<float> err_back;
        std::vector<float> fb_err;
        for(unsigned int i = 0; i < keypointsIN.size(); i++)
			pts.push_back(cv::Point2f(keypointsIN[i].first.pt.x,keypointsIN[i].first.pt.y));
        cv::calcOpticalFlowPyrLK(im_prev, im_gray, pts, nextPts, status, err);
        cv::calcOpticalFlowPyrLK(im_gray, im_prev, nextPts, pts_back, status_back, err_back);
        for(unsigned int i = 0; i < pts.size(); i++) {
            cv::Point2f v = pts_back[i]-pts[i];
            fb_err.push_back(sqrt(v.dot(v)));
        }
        for(unsigned int i = 0; i < status.size(); i++)
            status[i] = (fb_err[i] <= THR_FB) & status[i];

        keypointsTracked = std::vector<std::pair<cv::KeyPoint, int> >();
        for(unsigned int i = 0; i < pts.size(); i++) {
            std::pair<cv::KeyPoint, int> p = keypointsIN[i];
            if(status[i]) {
                p.first.pt = nextPts[i];
                keypointsTracked.push_back(p);
            }
        }
    } else keypointsTracked = std::vector<std::pair<cv::KeyPoint, int> >();
}

cv::Point2f rotate(cv::Point2f p, float rad) {
    if(rad == 0) return p;
    float s = sin(rad);
    float c = cos(rad);
    return cv::Point2f(c*p.x-s*p.y,s*p.x+c*p.y);
}

ArbitraryTracking::ArbitraryTracking() {
    detectorType = "Feature2D.BRISK";
    descriptorType = "Feature2D.BRISK";
    matcherType = "BruteForce-Hamming";
    thrOutlier = 20;
    thrConf = 0.75;
    thrRatio = 0.8;
    descriptorLength = 512;
    estimateScale = true;
    estimateRotation = true;
    nbInitialKeypoints = 0;
}

void ArbitraryTracking::initialise(cv::Mat im_gray0, cv::Point2f topleft, cv::Point2f bottomright) {
    detector = cv::Algorithm::create<cv::FeatureDetector>(detectorType.c_str());
    descriptorExtractor = cv::Algorithm::create<cv::DescriptorExtractor>(descriptorType.c_str());
    descriptorMatcher = cv::DescriptorMatcher::create(matcherType.c_str());
    std::vector<std::string> list;
    cv::Algorithm::getList(list);

    std::vector<cv::KeyPoint> keypoints;
    detector->detect(im_gray0, keypoints);

    std::vector<cv::KeyPoint> selected_keypoints;
    std::vector<cv::KeyPoint> background_keypoints;
    inout_rect(keypoints, topleft, bottomright, selected_keypoints, background_keypoints);
    descriptorExtractor->compute(im_gray0, selected_keypoints, selectedFeatures);

    if(selected_keypoints.size() == 0) {
        printf("No keypoints found in selection");
        return;
    }

    cv::Mat background_features;
    descriptorExtractor->compute(im_gray0, background_keypoints, background_features);

    selectedClasses = std::vector<int>();
    for(unsigned int i = 1; i <= selected_keypoints.size(); i++)
        selectedClasses.push_back(i);
    std::vector<int> backgroundClasses;
    for(unsigned int i = 0; i < background_keypoints.size(); i++)
        backgroundClasses.push_back(0);

    featuresDatabase = cv::Mat(background_features.rows+selectedFeatures.rows, std::max(background_features.cols,selectedFeatures.cols), background_features.type());
    if(background_features.cols > 0)
		background_features.copyTo(featuresDatabase(cv::Rect(0,0,background_features.cols, background_features.rows)));
    if(selectedFeatures.cols > 0)
		selectedFeatures.copyTo(featuresDatabase(cv::Rect(0,background_features.rows,selectedFeatures.cols, selectedFeatures.rows)));

    classesDatabase = std::vector<int>();
    for(unsigned int i = 0; i < backgroundClasses.size(); i++)
        classesDatabase.push_back(backgroundClasses[i]);
    for(unsigned int i = 0; i < selectedClasses.size(); i++)
        classesDatabase.push_back(selectedClasses[i]);

    squareForm = std::vector<std::vector<float> >();
    angles = std::vector<std::vector<float> >();
    for(unsigned int i = 0; i < selected_keypoints.size(); i++) {
        std::vector<float> lineSquare;
        std::vector<float> lineAngle;
        for(unsigned int j = 0; j < selected_keypoints.size(); j++) {
            float dx = selected_keypoints[j].pt.x-selected_keypoints[i].pt.x;
            float dy = selected_keypoints[j].pt.y-selected_keypoints[i].pt.y;
            lineSquare.push_back(sqrt(dx*dx+dy*dy));
            lineAngle.push_back(atan2(dy, dx));
        }
        squareForm.push_back(lineSquare);
        angles.push_back(lineAngle);
    }

    cv::Point2f center(0,0);
    for(unsigned int i = 0; i < selected_keypoints.size(); i++)
        center += selected_keypoints[i].pt;
    center *= (1.0/selected_keypoints.size());

    centerToTopLeft = topleft - center;
    centerToTopRight = cv::Point2f(bottomright.x, topleft.y) - center;
    centerToBottomRight = bottomright - center;
    centerToBottomLeft = cv::Point2f(topleft.x, bottomright.y) - center;

    springs = std::vector<cv::Point2f>();
    for(unsigned int i = 0; i < selected_keypoints.size(); i++)
        springs.push_back(selected_keypoints[i].pt - center);

    im_prev = im_gray0.clone();
    activeKeypoints = std::vector<std::pair<cv::KeyPoint,int> >();
    for(unsigned int i = 0; i < selected_keypoints.size(); i++)
        activeKeypoints.push_back(std::make_pair(selected_keypoints[i], selectedClasses[i]));
    nbInitialKeypoints = selected_keypoints.size();
}

typedef std::pair<int,int> PairInt;
typedef std::pair<float,int> PairFloat;

template<typename T>
bool comparatorPair ( const std::pair<T,int>& l, const std::pair<T,int>& r) {
    return l.first < r.first;
}

template<typename T>
bool comparatorPairDesc ( const std::pair<T,int>& l, const std::pair<T,int>& r) {
    return l.first > r.first;
}

template <typename T>
T sign(T t) {
    if( t == 0 )
        return T(0);
    else
        return (t < 0) ? T(-1) : T(1);
}

template<typename T>
T median(std::vector<T> list) {
    T val;
    std::nth_element(&list[0], &list[0]+list.size()/2, &list[0]+list.size());
    val = list[list.size()/2];
    if(list.size()%2==0) {
        std::nth_element(&list[0], &list[0]+list.size()/2-1, &list[0]+list.size());
        val = (val+list[list.size()/2-1])/2;
    }
    return val;
}

float findMinSymetric(const std::vector<std::vector<float> >& dist, const std::vector<bool>& used, int limit, int &i, int &j) {
    float min = dist[0][0];
    i = 0;
    j = 0;
    for(int x = 0; x < limit; x++) {
        if(!used[x]) {
            for(int y = x+1; y < limit; y++)
				if(!used[y] && dist[x][y] <= min) {
                    min = dist[x][y];
                    i = x;
                    j = y;
				}
		}
	} return min;
}

std::vector<Cluster> linkage(const std::vector<cv::Point2f>& list) {
    float inf = 10000000.0;
    std::vector<bool> used;
    for(unsigned int i = 0; i < 2*list.size(); i++)
        used.push_back(false);
    std::vector<std::vector<float> > dist;
    for(unsigned int i = 0; i < list.size(); i++) {
        std::vector<float> line;
        for(unsigned int j = 0; j < list.size(); j++) {
            if(i == j)
                line.push_back(inf);
            else {
                cv::Point2f p = list[i]-list[j];
                line.push_back(sqrt(p.dot(p)));
            }
        }
        for(unsigned int j = 0; j < list.size(); j++)
            line.push_back(inf);
        dist.push_back(line);
    }
    for(unsigned int i = 0; i < list.size(); i++) {
        std::vector<float> line;
        for(unsigned int j = 0; j < 2*list.size(); j++)
            line.push_back(inf);
        dist.push_back(line);
    }
    std::vector<Cluster> clusters;
    while(clusters.size() < list.size()-1) {
        int x, y;
        float min = findMinSymetric(dist, used, list.size()+clusters.size(), x, y);
        Cluster cluster;
        cluster.first = x;
        cluster.second = y;
        cluster.dist = min;
        cluster.num = (x < (int)list.size() ? 1 : clusters[x-list.size()].num) + (y < (int)list.size() ? 1 : clusters[y-list.size()].num);
        used[x] = true;
        used[y] = true;
        int limit = list.size()+clusters.size();
        for(int i = 0; i < limit; i++) {
            if(!used[i])
                dist[i][limit] = dist[limit][i] = std::min(dist[i][x], dist[i][y]);
        }
        clusters.push_back(cluster);
    }
    return clusters;
}

void fcluster_rec(std::vector<int>& data, const std::vector<Cluster>& clusters, float threshold, const Cluster& currentCluster, int& binId) {
    int startBin = binId;
    if(currentCluster.first >= (int)data.size())
        fcluster_rec(data, clusters, threshold, clusters[currentCluster.first - data.size()], binId);
    else data[currentCluster.first] = binId;

    if(startBin == binId && currentCluster.dist >= threshold)
        binId++;
    startBin = binId;

    if(currentCluster.second >= (int)data.size())
        fcluster_rec(data, clusters, threshold, clusters[currentCluster.second - data.size()], binId);
    else data[currentCluster.second] = binId;

    if(startBin == binId && currentCluster.dist >= threshold)
        binId++;
}

std::vector<int> binCount(const std::vector<int>& T) {
    std::vector<int> result;
    for(unsigned int i = 0; i < T.size(); i++) {
        while(T[i] >= (int)result.size())
            result.push_back(0);
        result[T[i]]++;
    }
    return result;
}

int argmax(const std::vector<int>& list) {
    int max = list[0];
    int id = 0;
    for(unsigned int i = 1; i < list.size(); i++)
		if(list[i] > max) {
            max = list[i];
            id = i;
    	} return id;
}

std::vector<int> fcluster(const std::vector<Cluster>& clusters, float threshold) {
    std::vector<int> data;
    for(unsigned int i = 0; i < clusters.size()+1; i++)
        data.push_back(0);
    int binId = 0;
    fcluster_rec(data, clusters, threshold, clusters[clusters.size()-1], binId);
    return data;
}

void ArbitraryTracking::estimate(const std::vector<std::pair<cv::KeyPoint, int> >& keypointsIN, cv::Point2f& center, float& scaleEstimate, float& medRot, std::vector<std::pair<cv::KeyPoint, int> >& keypoints) {
    center = cv::Point2f(NAN,NAN);
    scaleEstimate = NAN;
    medRot = NAN;
    if(keypointsIN.size() > 1) {
        std::vector<PairInt> list;
        for(unsigned int i = 0; i < keypointsIN.size(); i++)
            list.push_back(std::make_pair(keypointsIN[i].second, i));
        std::sort(&list[0], &list[0]+list.size(), comparatorPair<int>);
        for(unsigned int i = 0; i < list.size(); i++)
            keypoints.push_back(keypointsIN[list[i].second]);

        std::vector<int> ind1;
        std::vector<int> ind2;
        for(unsigned int i = 0; i < list.size(); i++)
		for(unsigned int j = 0; j < list.size(); j++) {
                if(i != j && keypoints[i].second != keypoints[j].second) {
                    ind1.push_back(i);
                    ind2.push_back(j);
                }
            }
        if(ind1.size() > 0) {
            std::vector<int> class_ind1;
            std::vector<int> class_ind2;
            std::vector<cv::KeyPoint> pts_ind1;
            std::vector<cv::KeyPoint> pts_ind2;
            for(unsigned int i = 0; i < ind1.size(); i++) {
                class_ind1.push_back(keypoints[ind1[i]].second-1);
                class_ind2.push_back(keypoints[ind2[i]].second-1);
                pts_ind1.push_back(keypoints[ind1[i]].first);
                pts_ind2.push_back(keypoints[ind2[i]].first);
            }

            std::vector<float> scaleChange;
            std::vector<float> angleDiffs;
            for(unsigned int i = 0; i < pts_ind1.size(); i++) {
                cv::Point2f p = pts_ind2[i].pt - pts_ind1[i].pt;
                float dist = sqrt(p.dot(p));
                float origDist = squareForm[class_ind1[i]][class_ind2[i]];
                scaleChange.push_back(dist/origDist);
                float angle = atan2(p.y, p.x);
                float origAngle = angles[class_ind1[i]][class_ind2[i]];
                float angleDiff = angle - origAngle;
                if(fabs(angleDiff) > CV_PI)
                    angleDiff -= sign(angleDiff) * 2 * CV_PI;
                angleDiffs.push_back(angleDiff);
            }
            scaleEstimate = median(scaleChange);
            if(!estimateScale)
                scaleEstimate = 1;
            medRot = median(angleDiffs);
            if(!estimateRotation)
                medRot = 0;
            votes = std::vector<cv::Point2f>();
            for(unsigned int i = 0; i < keypoints.size(); i++)
                votes.push_back(keypoints[i].first.pt - scaleEstimate * rotate(springs[keypoints[i].second-1], medRot));
            std::vector<Cluster> linkageData = linkage(votes);

            std::vector<int> T = fcluster(linkageData, thrOutlier);
            std::vector<int> cnt = binCount(T);
            int Cmax = argmax(cnt);
            outliers = std::vector<std::pair<cv::KeyPoint, int> >();
            std::vector<std::pair<cv::KeyPoint, int> > newKeypoints;
            std::vector<cv::Point2f> newVotes;
            for(unsigned int i = 0; i < keypoints.size(); i++) {
                if(T[i] != Cmax)
                    outliers.push_back(keypoints[i]);
                else {
                    newKeypoints.push_back(keypoints[i]);
                    newVotes.push_back(votes[i]);
                }
            }
            keypoints = newKeypoints;

            center = cv::Point2f(0,0);
            for(unsigned int i = 0; i < newVotes.size(); i++)
                center += newVotes[i];
            center *= (1.0/newVotes.size());
        }
    }
}

std::vector<bool> in1d(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<bool> result;
    for(unsigned int i = 0; i < a.size(); i++) {
        bool found = false;
        for(unsigned int j = 0; j < b.size(); j++)
			if(a[i] == b[j]) {
                found = true;
                break;
            }
        result.push_back(found);
    }
    return result;
}

void ArbitraryTracking::processFrame(cv::Mat im_gray) {
    trackedKeypoints = std::vector<std::pair<cv::KeyPoint, int> >();
    std::vector<unsigned char> status;
    track(im_prev, im_gray, activeKeypoints, trackedKeypoints, status);

    cv::Point2f center;
    float scaleEstimate;
    float rotationEstimate;
    std::vector<std::pair<cv::KeyPoint, int> > trackedKeypoints2;
    estimate(trackedKeypoints, center, scaleEstimate, rotationEstimate, trackedKeypoints2);
    trackedKeypoints = trackedKeypoints2;

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat features;
    detector->detect(im_gray, keypoints);
    descriptorExtractor->compute(im_gray, keypoints, features);

    activeKeypoints = std::vector<std::pair<cv::KeyPoint, int> >();

    for(unsigned int i = 0; i < keypoints.size(); i++) {
        cv::KeyPoint keypoint = keypoints[i];
        std::vector<cv::DMatch> matches;
        descriptorMatcher->match(featuresDatabase,features.row(i), matches);
        std::vector<float> combined;
        for(unsigned int j = 0; j < matches.size(); j++)
            combined.push_back(1 - matches[j].distance / descriptorLength);

        std::vector<int>& classes = classesDatabase;
		std::vector<PairFloat> sorted_conf;
        for(unsigned int j = 0; j < combined.size(); j++)
            sorted_conf.push_back(std::make_pair(combined[j], j));
        std::sort(&sorted_conf[0], &sorted_conf[0]+sorted_conf.size(), comparatorPairDesc<float>);
        int bestInd = sorted_conf[0].second;
        int secondBestInd = sorted_conf[1].second;

        float ratio = (1-combined[bestInd]) / (1-combined[secondBestInd]);

        int keypoint_class = classes[bestInd];

        if(ratio < thrRatio && combined[bestInd] > thrConf && keypoint_class != 0)
            activeKeypoints.push_back(std::make_pair(keypoint, keypoint_class));

        if(!(isnan(center.x) | isnan(center.y))) {
            std::vector<cv::DMatch> matches;
            descriptorMatcher->match(selectedFeatures, features.row(i), matches);
            std::vector<float> confidences;
            for(unsigned int i = 0; i < matches.size(); i++)
                confidences.push_back(1 - matches[i].distance / descriptorLength);
            cv::Point2f relative_location = keypoint.pt - center;
            std::vector<float> displacements;
            for(unsigned int i = 0; i < springs.size(); i++) {
                cv::Point2f p = (scaleEstimate * rotate(springs[i], -rotationEstimate) - relative_location);
                displacements.push_back(sqrt(p.dot(p)));
            }
            std::vector<float> combined;
            for(unsigned int i = 0; i < confidences.size(); i++)
                combined.push_back((displacements[i] < thrOutlier)*confidences[i]);

            std::vector<int>& classes = selectedClasses;
            std::vector<PairFloat> sorted_conf;
            for(unsigned int i = 0; i < combined.size(); i++)
                sorted_conf.push_back(std::make_pair(combined[i], i));
            std::sort(&sorted_conf[0], &sorted_conf[0]+sorted_conf.size(), comparatorPairDesc<float>);
            int bestInd = sorted_conf[0].second;
            int secondBestInd = sorted_conf[1].second;
            float ratio = (1-combined[bestInd]) / (1-combined[secondBestInd]);
            int keypoint_class = classes[bestInd];
            if(ratio < thrRatio && combined[bestInd] > thrConf && keypoint_class != 0) {
                for(int i = activeKeypoints.size()-1; i >= 0; i--)
                    if(activeKeypoints[i].second == keypoint_class)
                        activeKeypoints.erase(activeKeypoints.begin()+i);
                activeKeypoints.push_back(std::make_pair(keypoint, keypoint_class));
            }
        }
    }
    if(trackedKeypoints.size() > 0) {
        std::vector<int> tracked_classes;
        for(unsigned int i = 0; i < trackedKeypoints.size(); i++)
            tracked_classes.push_back(trackedKeypoints[i].second);
        if(activeKeypoints.size() > 0) {
            std::vector<int> associated_classes;
            for(unsigned int i = 0; i < activeKeypoints.size(); i++)
                associated_classes.push_back(activeKeypoints[i].second);
            std::vector<bool> notmissing = in1d(tracked_classes, associated_classes);
            for(unsigned int i = 0; i < trackedKeypoints.size(); i++)
                if(!notmissing[i])
                    activeKeypoints.push_back(trackedKeypoints[i]);
        }
        else activeKeypoints = trackedKeypoints;
    }
    std::vector<std::pair<cv::KeyPoint, int> > activeKeypointsBefore = activeKeypoints;
    im_prev = im_gray;
    topLeft = cv::Point2f(NAN,NAN);
    topRight = cv::Point2f(NAN,NAN);
    bottomLeft = cv::Point2f(NAN,NAN);
    bottomRight = cv::Point2f(NAN,NAN);

    boundingbox = cv::Rect_<float>(NAN,NAN,NAN,NAN);
    hasResult = false;
    if(!(isnan(center.x) | isnan(center.y)) && activeKeypoints.size() > nbInitialKeypoints / 10) {
        hasResult = true;
        topLeft = center + scaleEstimate*rotate(centerToTopLeft, rotationEstimate);
        topRight = center + scaleEstimate*rotate(centerToTopRight, rotationEstimate);
        bottomLeft = center + scaleEstimate*rotate(centerToBottomLeft, rotationEstimate);
        bottomRight = center + scaleEstimate*rotate(centerToBottomRight, rotationEstimate);
        float minx = std::min(std::min(topLeft.x,topRight.x),std::min(bottomRight.x, bottomLeft.x));
        float miny = std::min(std::min(topLeft.y,topRight.y),std::min(bottomRight.y, bottomLeft.y));
        float maxx = std::max(std::max(topLeft.x,topRight.x),std::max(bottomRight.x, bottomLeft.x));
        float maxy = std::max(std::max(topLeft.y,topRight.y),std::max(bottomRight.y, bottomLeft.y));
        boundingbox = cv::Rect_<float>(minx, miny, maxx-minx, maxy-miny);
    }
}
