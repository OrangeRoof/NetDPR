// side by side derotation
#include <string>
#include <sstream>
#include <errno.h>
#include <opencv2/opencv.hpp>


// show rotated and non-rotated videos side by side. most code here is similar
// to derot, so comments will be heavy in different areas.
int main(int argc, const char* argv[]) {
    auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

    // ./bin FILE X Y R RPM OUTFILE
    // return 255 if not the case that all six arguments are present
    if (argc != 7)
        return(-1);
    std::string filename = std::string(argv[1]);
    auto circ = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    int radii = atoi(argv[4]);
    double rpm = strtod(argv[5], NULL);
    std::string outfn = std::string(argv[6]);
    std::string tmpfn = std::string(outfn);
    auto f = tmpfn.find_last_of("/");
    f = (f == std::string::npos) ? 0 : f+1;
    tmpfn.insert(f, "TMP");

    if (!rpm || errno == ERANGE)
        return(-1);
    auto vid = cv::VideoCapture(filename);
    // create a matrix for storing a single video frame
    cv::Mat vid_frame;
    // if cannot read, return 254.
    if (!vid.read(vid_frame))
        return(-2);

    double fps = vid.get(cv::CAP_PROP_FPS);
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    cv::circle(center_mask, circ, radii, 255, -1);
    cv::bitwise_not(center_mask, center_mask);

    // output size is just two circle of interest areas, side by side
    // circle of interest areas which jut out of the frame: bound by frame limits
    // four rectangles specifying copy regions for reg(ular)/derot(ated),
    // both from in (original frame) and out (output frame)
    // 20 is to allow 20 pixels for text at the top of the frame
    auto regin = cv::Rect(
        cv::Point(circ.x - radii > 0 ? circ.x - radii : 0,
            circ.y - radii > 0 ? circ.y - radii : 0),
        cv::Point(circ.x + radii < dims.width ? circ.x + radii : dims.width,
            circ.y + radii < dims.height ? circ.y + radii : dims.height));
    auto regout = cv::Rect(0, 30, regin.width, regin.height);
    auto derotin = cv::Rect(regin);
    auto derotout = cv::Rect(regin.width + 10, 30, derotin.width, derotin.height);
    // max is unnecessary here
    auto outdims = cv::Size(regin.width + derotin.width + 10, 30 + std::max(regin.height, derotin.height));
    // set up output frame to write to
    cv::Mat out_frame = cv::Mat::zeros(outdims, vid_frame.type());
    auto vidout = cv::VideoWriter(tmpfn, codec, fps, outdims);
    double i = 0.0;
    double dtheta = -6.0 * rpm / fps;

    // return 254 if output video cannot be opened, for whatever reason
    if (!vidout.isOpened())
        return(-2);

    // stringstream to manage rpm precision
    std::ostringstream strrpm;
    strrpm.precision(2);
    strrpm << std::fixed << rpm;
    // equivalent to basename (get filename from path)
    // construct overlay text
    std::string overlaytxtup = "Derotated at " + strrpm.str() + " rpm.";
    std::string overlaytxtlow = "Generated using diyrot.epss.ucla.edu";
    // calculate origin, white color
    auto origup = cv::Point(0, out_frame.rows-30);
    auto origlow = cv::Point(0, out_frame.rows-5);
    auto origleft = cv::Point(out_frame.cols / 4 - 50, 20);
    auto origright = cv::Point(out_frame.cols * 3 / 4 - 60, 20);
    auto color = cv::Scalar(255, 255, 255, 127.5);

    do {
        i += dtheta;
        vid_frame(regin).copyTo(out_frame(regout));
        cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i, 1.0), dims);
        cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
        vid_frame(derotin).copyTo(out_frame(derotout));
        cv::putText(out_frame, overlaytxtup, origup, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        cv::putText(out_frame, overlaytxtlow, origlow, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        cv::putText(out_frame, "Original", origleft, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 1, cv::LINE_AA);
        cv::putText(out_frame, "Derotated", origright, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 1, cv::LINE_AA);
        // write vid_frame to the video. equivalent to vidout.write(vid_frame)
        vidout << out_frame;
    } while(vid.read(vid_frame));

    vidout.release();
    rename(tmpfn.c_str(), outfn.c_str());
    return EXIT_SUCCESS;
}
