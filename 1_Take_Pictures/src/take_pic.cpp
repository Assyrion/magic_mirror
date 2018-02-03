#include <raspicam/raspicam_cv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

#define IM_WIDTH 100
#define IM_HEIGHT 100

int main(int argc, char **argv)
{
    raspicam::RaspiCam_Cv Camera; // Camera Object

    // Set camera params
    Camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
    Camera.set(CV_CAP_PROP_FORMAT, CV_8UC3); // For color
    Camera.set(CV_CAP_PROP_EXPOSURE, -1); //-1 is auto, values range from 0 to 100

    cv::Mat frame, gray, face,face_resized;
    std::vector< cv::Rect_<int> > faces;

    // Open camera
    std::cout << "Opening camera..." << std::endl;
    if (!Camera.open()) {
        std::cerr << "Error opening camera!" << std::endl;
        return -1;
    }

    cv::CascadeClassifier face_cascade;
    std::string fn_haar = "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml";
    //load face model
    if(!face_cascade.load(fn_haar)) {
        std::cout <<"(E) face cascade model not loaded :"+fn_haar+"\n";
        return -1;
    }
    std::cout << "(vision) load model : ok" << std::endl;
    cv::namedWindow("camcvWin", cv::WINDOW_AUTOSIZE);

    int count = 0;

    std::cout << "Press enter when you're ready..." << std::endl;
    std::cin.get();

    // Start capturing
    for (;;) {
        Camera.grab();
        Camera.retrieve(frame);
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // and equalize Histo (as model pictures)
        cv::equalizeHist( gray, gray);
        // detect faces
        face_cascade.detectMultiScale(gray, faces, 1.1, 3, 0|CV_HAAR_SCALE_IMAGE, cv::Size(80,80));

        for(int i = 0; i < faces.size(); i++) {
            // crop face
            cv::Rect face_i = faces[i];

            face = gray(face_i);
            //  resized face and display it
            cv::resize(face, face_resized, cv::Size(IM_WIDTH, IM_HEIGHT), 1.0, 1.0, cv::INTER_CUBIC/*cv::INTER_NN*/); //INTER_CUBIC);
            cv::imshow("camcvWin", face_resized);
            if (cv::waitKey(1) > 0) {
                break;
            }
            std::time_t rawtime;
            struct tm* timeinfo;
            char buffer[80];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            std::strftime(buffer, sizeof(buffer), "%F_%H-%M-%S", timeinfo);
            std::string dateTimeStr{buffer};

            std::string filename = dateTimeStr + "_" + std::to_string(count) + ".jpg";

            cv::imwrite("./Pics/"+filename, face_resized);
            count++;
        }
    } // end for

    std::cout << "Stopping camera.." << std::endl;
    Camera.release();
    return 0;
}
