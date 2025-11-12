

#include <iostream>
#include <vector>
#include <cmath>

using namespace std;
using namespace cv;

// ---- Structs for motion and trajectory ----
struct TransformParam {
    double dx, dy, da;
    TransformParam() {}
    TransformParam(double _dx, double _dy, double _da)
        : dx(_dx), dy(_dy), da(_da) {}
};

struct Trajectory {
    double x, y, a;
    Trajectory() {}
    Trajectory(double _x, double _y, double _a)
        : x(_x), y(_y), a(_a) {}
};

// ---- Moving average smoother ----
Trajectory movingAverage(const vector<Trajectory> &traj, int radius, int i) {
    double sum_x = 0, sum_y = 0, sum_a = 0;
    int count = 0;
    for (int j = -radius; j <= radius; j++) {
        if (i + j >= 0 && i + j < (int)traj.size()) {
            sum_x += traj[i + j].x;
            sum_y += traj[i + j].y;
            sum_a += traj[i + j].a;
            count++;
        }
    }
    return Trajectory(sum_x / count, sum_y / count, sum_a / count);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cout << "Usage: ./stabilize <input_video>" << endl;
        return 0;
    }

    string inputPath = argv[1];
    VideoCapture cap(inputPath);
    if (!cap.isOpened()) {
        cerr << "Error: cannot open video file " << inputPath << endl;
        return -1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    int w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int totalFrames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);

    cout << "Video Info: " << w << "x" << h << " @" << fps << " FPS, "
         << totalFrames << " frames" << endl;

    Mat prev, prevGray;
    cap >> prev;
    if (prev.empty()) {
        cerr << "Error: no frames in video." << endl;
        return -1;
    }
    cvtColor(prev, prevGray, cv::COLOR_BGR2GRAY);

    vector<TransformParam> prev_to_cur_transform;

    // ---- STEP 1: Estimate motion between frames ----
    cout << "Estimating motion..." << endl;
    while (true) {
        Mat curr, currGray;
        cap >> curr;
        if (curr.empty()) break;

        cvtColor(curr, currGray, cv::COLOR_BGR2GRAY);

        vector<Point2f> prevCorners, currCorners;
        goodFeaturesToTrack(prevGray, prevCorners, 200, 0.01, 30);

        vector<uchar> status;
        vector<float> err;
        calcOpticalFlowPyrLK(prevGray, currGray, prevCorners, currCorners, status, err);

        vector<Point2f> prevGood, currGood;
        for (size_t i = 0; i < status.size(); i++) {
            if (status[i]) {
                prevGood.push_back(prevCorners[i]);
                currGood.push_back(currCorners[i]);
            }
        }

        Mat T = estimateAffinePartial2D(prevGood, currGood);
        if (T.empty()) {
            T = Mat::eye(2, 3, CV_64F);
        }

        double dx = T.at<double>(0, 2);
        double dy = T.at<double>(1, 2);
        double da = atan2(T.at<double>(1, 0), T.at<double>(0, 0));
        prev_to_cur_transform.push_back(TransformParam(dx, dy, da));

        currGray.copyTo(prevGray);
    }

    cout << "Frame transforms computed: " << prev_to_cur_transform.size() << endl;

    // ---- STEP 2: Accumulate to get trajectory ----
    vector<Trajectory> trajectory;
    double x = 0, y = 0, a = 0;
    for (size_t i = 0; i < prev_to_cur_transform.size(); i++) {
        x += prev_to_cur_transform[i].dx;
        y += prev_to_cur_transform[i].dy;
        a += prev_to_cur_transform[i].da;
        trajectory.push_back(Trajectory(x, y, a));
    }

    // ---- STEP 3: Smooth trajectory ----
    cout << "Smoothing trajectory..." << endl;
    int SMOOTHING_RADIUS = 30;
    vector<Trajectory> smoothed_traj;
    for (size_t i = 0; i < trajectory.size(); i++) {
        smoothed_traj.push_back(movingAverage(trajectory, SMOOTHING_RADIUS, i));
    }

    // ---- STEP 4: Generate new transforms ----
    vector<TransformParam> new_transforms;
    x = y = a = 0;
    for (size_t i = 0; i < prev_to_cur_transform.size(); i++) {
        x += prev_to_cur_transform[i].dx;
        y += prev_to_cur_transform[i].dy;
        a += prev_to_cur_transform[i].da;

        double diff_x = smoothed_traj[i].x - x;
        double diff_y = smoothed_traj[i].y - y;
        double diff_a = smoothed_traj[i].a - a;

        double dx = prev_to_cur_transform[i].dx + diff_x;
        double dy = prev_to_cur_transform[i].dy + diff_y;
        double da = prev_to_cur_transform[i].da + diff_a;

        new_transforms.push_back(TransformParam(dx, dy, da));
    }

    // ---- STEP 5: Apply new transforms ----
    cout << "Applying transforms and writing stabilized video..." << endl;
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    Mat curr, currGray;
    cap >> curr;
    cvtColor(curr, currGray, cv::COLOR_BGR2GRAY);

    VideoWriter writer("stabilized_output.avi",
                       VideoWriter::fourcc('M', 'J', 'P', 'G'),
                       fps,
                       Size(w, h));

    Mat last_T = Mat::eye(2, 3, CV_64F);
    for (size_t i = 0; i < new_transforms.size(); i++) {
        Mat frame, frame_stabilized, T(2, 3, CV_64F);
        cap >> frame;
        if (frame.empty()) break;

        double dx = new_transforms[i].dx;
        double dy = new_transforms[i].dy;
        double da = new_transforms[i].da;

        T.at<double>(0, 0) = cos(da);
        T.at<double>(0, 1) = -sin(da);
        T.at<double>(1, 0) = sin(da);
        T.at<double>(1, 1) = cos(da);
        T.at<double>(0, 2) = dx;
        T.at<double>(1, 2) = dy;

        warpAffine(frame, frame_stabilized, T, frame.size());
        writer.write(frame_stabilized);

        // Optional: display both side by side
        Mat canvas(frame.rows, frame.cols * 2 + 10, frame.type());
        frame.copyTo(canvas(Rect(0, 0, frame.cols, frame.rows)));
        frame_stabilized.copyTo(canvas(Rect(frame.cols + 10, 0, frame.cols, frame.rows)));
        imshow("Before and After", canvas);
        if (waitKey(10) == 27) break; // ESC to exit
    }

    cout << "Stabilization complete. Output: stabilized_output.avi" << endl;
    return 0;
}
