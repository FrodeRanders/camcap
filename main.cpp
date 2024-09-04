#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>

constexpr int MAX_NUMBER_OF_RETRIES = 10;
constexpr int MINIMAL_WAIT_TIME = 2; // seconds

// See https://opencvguide.readthedocs.io/en/latest/opencvcpp/basics.html

std::string timestamp() {
    auto now = std::chrono::system_clock::now();

    auto coarse = std::chrono::system_clock::to_time_t(now);
    auto fine = time_point_cast<std::chrono::milliseconds>(now);

    char buffer[sizeof "9999-12-31 23:59:59.999"];
    std::snprintf(buffer + std::strftime(buffer, sizeof buffer - 3, "%F %T.", std::localtime(&coarse)),
                  4, "%03lu", (unsigned long) fine.time_since_epoch().count() % 1000);

    return {buffer};
}

// Update process to return a bool indicating if processing should continue
bool process(cv::VideoCapture &capture, int &numRetriesLeft, int &waitTime) {
    // Establish mask
    // cv::Mat mask;

    cv::Mat baseline;
    double baselineAlpha = 0.5; // Weight of baseline when mixing (0..1)

    cv::Mat capturedFrame;
    int countDown = 0;

    for (;;) {
        if (!capture.grab()) {
            std::cout << "Failed to grab frame, attempting to reconnect in " << waitTime << "seconds..." << std::endl;
            return false;  // Signal the need to reconnect
        }

        if (!capture.retrieve(capturedFrame)) {
            std::cout << "Failed to retrieve grabbed frame, attempting to reconnect in " << waitTime << "seconds..." << std::endl;
            return false;  // Signal the need to reconnect
        }

        if (capturedFrame.empty()) {
            std::cout << "Grabbed frame was empty, attempting to reconnect in " << waitTime << "seconds..." << std::endl;
            return false;  // Signal the need to reconnect
        }

        // Reset error handling
        numRetriesLeft = MAX_NUMBER_OF_RETRIES;
        waitTime = MINIMAL_WAIT_TIME;

        // Apply mask (if needed)
        // cv::bitwise_and(frame, frame, mask);

        // Convert to grayscale
        cv::Mat greyFrame;
        cv::cvtColor(capturedFrame, greyFrame, cv::COLOR_RGB2GRAY);

        // Establish baseline as captured greyscale frame (first time)
        if (baseline.empty()) {
            std::cout << "Establishing baseline..." << std::endl;
            baseline = greyFrame;
            continue;
        }

        // Update baseline by mixing last baseline with captured greyscale frame
        cv::addWeighted(baseline, baselineAlpha, greyFrame, (1 - baselineAlpha), 0, baseline);

        // Calculate difference between last captured greyscale frame and baseline
        cv::Mat diff;
        cv::absdiff(greyFrame, baseline, diff);

        // Apply threshold
        cv::threshold(diff, diff, 30, 255, cv::THRESH_BINARY);

        // Calculate a mean over all pixels in diff matrix
        cv::Scalar_<double> _mean = cv::mean(diff);
        double diffScore = _mean[0];

        std::string now = timestamp();
        std::cout << now << " score: " << diffScore << std::endl;

        bool saveFrames = false;

        if (diffScore > 0.1) {  // An arbitrarily chosen threshold
            saveFrames = true;
            countDown = 10;
        }

        if (saveFrames || countDown > 0) {
            --countDown;
            std::string name = "images/" + now + " (" + std::to_string(diffScore) + ").jpg";
            cv::imwrite(name, capturedFrame);
        }

        bool showImage = false;
        if (showImage) {
            cv::imshow("Camera", capturedFrame);
        }

        if (cv::waitKey(30) >= 0) {
            break;
        }
    }

    return true;  // Continue processing normally
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <url to camera>" << std::endl;
        return 1;
    }

    std::string url = std::string(argv[1]);

    int numRetriesLeft = MAX_NUMBER_OF_RETRIES;
    int waitTime = MINIMAL_WAIT_TIME;

    do {
        cv::VideoCapture capture(url, cv::CAP_FFMPEG);
        try {
            if (!capture.isOpened()) {
                std::cout << "Could not open stream, retrying in " << waitTime << " seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(waitTime));  // Wait before retrying
                waitTime *= 2;
                continue;
            }

            std::cout << "Connected to camera: "
                    << capture.getBackendName() << " "
                    << capture.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
                    << capture.get(cv::CAP_PROP_FRAME_HEIGHT) << " ("
                    << capture.get(cv::CAP_PROP_FPS) << " fps)" << std::endl;

            // Process the stream until an error occurs or the user exits
            if (!process(capture, numRetriesLeft, waitTime)) {
                std::cout << "Stream interrupted, attempting to reconnect..." << std::endl;
            } else {
                break;  // Exit loop if processing was successful and user exited
            }
        } catch (const std::exception &e) {
            std::cout << "Failed: " << e.what() << std::endl;
            capture.release();
        }

        std::this_thread::sleep_for(std::chrono::seconds(waitTime));  // Wait before retrying
        waitTime *= 2;

    } while (--numRetriesLeft > 0);

    cv::destroyAllWindows();
    return 0;
}