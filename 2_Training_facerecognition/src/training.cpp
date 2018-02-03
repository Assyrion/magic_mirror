#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <dirent.h>

std::string pic_path;
const std::string model_file = "./training_model/model.yaml";

void processFile(const std::string& filename, std::vector<cv::Mat>& images, std::vector<int>& labels)
{
    std::string label = filename.substr(pic_path.size()+1,
                                        filename.find_last_of('/')-pic_path.size()-1);
    std::cout << "(training) extract img : " << filename << " label : " << label << std::endl;

    cv::Mat img = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    cv::imshow("image_to_train", img);

    images.push_back(img);
    labels.push_back(stoi(label));
}

bool processDir(const std::string& dirname, std::vector<cv::Mat>& images, std::vector<int>& labels)
{
    auto dir = opendir(dirname.c_str());
    if(dir == nullptr) {
        std::cout << "(training) unable to open dir" << std::endl;
        return false;
    }
    auto entity = readdir(dir);
    while(entity != nullptr) {
        if(entity->d_type == DT_DIR && entity->d_name[0] != '.') {
            processDir(dirname+'/'+entity->d_name, images, labels);
        } else if(entity->d_type == DT_REG) {
            processFile(dirname+'/'+entity->d_name, images, labels);
        }
        entity = readdir(dir);
    }

    return true;
}

int main(int argc, char **argv)
{    
    cv::Ptr<cv::FaceRecognizer> model = cv::createFisherFaceRecognizer(); //Eigenfaces, Fisherfaces, LBPH

    cv::namedWindow("image_to_train", cv::WINDOW_AUTOSIZE);

    std::string fn_haar;
    int im_width;		// image width
    int im_height;		// image height

    std::vector<cv::Mat> images;
    std::vector<int> labels;

    std::cout << "(training) People initialized" << argv[1] << std::endl;

    pic_path = argv[1];

    // Get the path to your CSV
    fn_haar = "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml";

    if(!processDir(pic_path, images, labels)) {
        std::cout << "(training) failed getting files" << std::endl;
        return 0;
    }

    // get heigh, witdh of 1st images--> must be the same
    im_width = images[0].cols;
    im_height = images[0].rows;
    std::cout << "(training) taille images "+std::to_string(im_width)+" "+std::to_string(im_height)+" ok" << std::endl;

    //
    // Create a FaceRecognizer and train it on the given images:
    //

    // train the model with your nice collection of pictures
    std::cout << "(training) start train images" << std::endl;
    model->train(images, labels);
    std::cout << "(training) train images : ok" << std::endl;
    model->save(model_file);
    std::cout << "(training) save model : ok" << std::endl;

    return 0;
}
