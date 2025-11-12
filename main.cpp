#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace cv;

static string base_string(const string &path) {
    size_t slash = path.find_last_of("/\\");
    size_t dot = path.find_last_of('.');
    size_t start = (slash == string::npos) ? 0 : slash + 1;
    size_t end = (dot == string::npos || dot < start) ? path.size() : dot;
    return path.substr(start, end - start);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        cout << "Usage:\n  ./main <original_video> [stabilized_video]\n";
        return 0;
    }

    string origPath = argv[1];
    string stablePath;
    if (argc >= 3) stablePath = argv[2];

    VideoCapture cap1(origPath);
    VideoCapture cap2;
    if (!stablePath.empty()) cap2.open(stablePath);

    if (!cap1.isOpened()) {
        cerr << "Error: cannot open original video: " << origPath << endl;
        return -1;
    }

    if (!cap2.isOpened()) {
        cerr << "Warning: cannot open stabilized video: "
             << (stablePath.empty() ? "<none provided>" : stablePath)
             << "\n         Will duplicate original frames for comparison.\n";
    }

    double fps1 = cap1.get(cv::CAP_PROP_FPS);
    if (fps1 <= 0) fps1 = 25.0; // safe fallback
    int w1 = static_cast<int>(cap1.get(cv::CAP_PROP_FRAME_WIDTH));
    int h1 = static_cast<int>(cap1.get(cv::CAP_PROP_FRAME_HEIGHT));

    // Output filename: before_after_output.avi
    string outName = "before_after_output.avi";
    Size outSize(w1 * 2 + 10, h1);

    VideoWriter writer(outName,
                       VideoWriter::fourcc('M','J','P','G'),
                       fps1,
                       outSize,
                       true);
    if (!writer.isOpened()) {
        cerr << "Error: cannot open VideoWriter to write " << outName << endl;
        return -1;
    }

    Mat frame1, frame2;
    while (true) {
        if (!cap1.read(frame1)) break; // read returns bool
        if (cap2.isOpened()) {
            if (!cap2.read(frame2)) {
                // if stabilized video shorter, duplicate original
                frame1.copyTo(frame2);
            }
        } else {
            frame1.copyTo(frame2);
        }

        if (frame1.size() != frame2.size()) {
            resize(frame2, frame2, frame1.size());
        }

        Mat canvas = Mat::zeros(frame1.rows, frame1.cols * 2 + 10, frame1.type());
        frame1.copyTo(canvas(Rect(0, 0, frame1.cols, frame1.rows)));
        frame2.copyTo(canvas(Rect(frame1.cols + 10, 0, frame1.cols, frame1.rows)));

        writer.write(canvas);

        // Optional: display if your OpenCV build supports GUI.
        // imshow("Before and After", canvas);
        // if (waitKey(10) == 27) break;
    }

    cout << "Done. Output saved to: " << outName << endl;
    return 0;
}
