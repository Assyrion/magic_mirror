extern "C"{
#include "MQTTClient.h"
#include "MQTTClientPersistence.h"
}

#include <raspicam/raspicam_cv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#define IM_WIDTH 100
#define IM_HEIGHT 100

// MQTT defines
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "recognizer"
#define TOPIC       "hello_topic"
#define QOS         0
#define TIMEOUT     10000L

const std::string config_file = "/home/pi/MAGIC_MIRROR/3_FaceRecognition/config/people.csv";

void read_csv_config(std::map<int,std::string> &people_map, char separator = ';') {
    std::ifstream file(config_file.c_str(), std::ifstream::in);
    if (!file) {
        std::string error_message = "(E) No valid input file was given, please check the given filename." + config_file;
        CV_Error(CV_StsBadArg, error_message);
    }
    std::string line, label, name;
    while(getline(file, line)) {
        std::stringstream liness(line);
        getline(liness, label, separator);
        getline(liness, name);
        if(!label.empty() && !name.empty()) {
            people_map[std::stoi(label)] = name;
            std::cout << label << " " << name << std::endl;
        }
    }
}

void say_this(std::string const& message)
{
    std::string cmd = "/home/pi/TextToSpeech/speech2.sh \""+message+"\"";
    std::system(cmd.c_str());
}

void mqtt_publish(MQTTClient* client, std::string const& payload)
{
    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    pubmsg.payload = (char*)payload.c_str();
    pubmsg.payloadlen = payload.size();

    std::cout << "publish : " << payload << std::endl;
    MQTTClient_publishMessage(*client, TOPIC, &pubmsg, &token);
}

int main(int argc, char **argv)
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 0;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        std::cout << "Failed to connect" << std::endl;
        exit(-1);
    }

    raspicam::RaspiCam_Cv Camera; // Camera Object

    cv::CascadeClassifier face_cascade;
    cv::Ptr<cv::FaceRecognizer> model = cv::createFisherFaceRecognizer(); //Eigenfaces, Fisherfaces
    //Ptr<FaceRecognizer> model = createLBPHFaceRecognizer(); //Eigenfaces, Fisherfaces

    std::string fn_haar;
    int PREDICTION_SEUIL ;
    char key;

    int bHisto;
    std::vector< cv::Rect_<int> > faces;

    cv::Mat gray,frame,face,face_resized;

    int nCount=0;

    /////////////////////////////////
    // BEGIN OF FACE RECO vision
    /////////////////////////////////

    //
    // see thinkrpi.wordpress.com, articles on Magic Mirror to understand this command line and parameters
    //
    std::cout<<"start\n";
    if (argc < 4) {
        std::cout << "usage: " << argv[0] << " ext_files  seuil(opt) \n files.ext histo(0/1) 5000 \n" << std::endl;
        exit(1);
    }

    int display = 0;

    // set value by default for prediction treshold = minimum value to recognize
    if (argc>=3) {
        std::cout << "(vision) prediction treeshold = 4500.0 by default" << std::endl;
        PREDICTION_SEUIL = 4500.0;
    }
    if (argc>=4) PREDICTION_SEUIL = atoi(argv[3]);
    if (argc>=5) display = atoi(argv[4]);

    // do we do a color histogram equalization ?
    bHisto=atoi(argv[2]);

    std::map<int,std::string> people_map;
    read_csv_config(people_map);

    // vision...

    // Note : /!\ change with your opencv path
    fn_haar = "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml";

    //
    // Create a FaceRecognizer and train it on the given images:
    //

    // this a Eigen model, but you could replace with Fisher model (in this case
    // threshold value should be lower) (try)

    //	Fisherfaces model;

    // load the model
    std::cout << "(vision) start loading model" << std::endl;
    model->load(argv[1]);

    // load face model
    if (!face_cascade.load(fn_haar)) {
        std::cout <<"(E) face cascade model not loaded :"+fn_haar << std::endl;
        return -1;
    }
    std::cout << "(vision) load model : ok" << std::endl;

    /////////////////////////////////
    // END OF FACE RECO vision
    /////////////////////////////////

    // Set camera params
    Camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
    Camera.set(CV_CAP_PROP_FORMAT, CV_8UC3); // For color
    //Camera.set(CV_CAP_PROP_BRIGHTNESS, 50);
    Camera.set(CV_CAP_PROP_EXPOSURE, -1); //-1 is auto, values range from 0 to 100
    //Camera.set(CV_CAP_PROP_WHITE_BALANCE_RED_V, -1); //values range from 0 to 100, -1 auto whitebalance
    //Camera.set(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, -1); //values range from 0 to 100,  -1 auto whitebalance

    // Open camera
    std::cout << "Opening camera..." << std::endl;
    if (!Camera.open()) {
        std::cerr << "Error opening camera!" << std::endl; return -1;
    } // Start capturing
    if(display == 1) {
        cv::namedWindow("camcvWin", cv::WINDOW_AUTOSIZE);
        cv::namedWindow("ROI", cv::WINDOW_AUTOSIZE);
    }

    //    say_this("bonjour. je suis le magic mirror dantoine. je vais essayer de reconnaitreta tête, maissitu es trop moche, je n'y arriverai pas. Mort de rire.");

    int prediction = -1, last_prediction = -1;
    std::array<int, 10> vector_pred;
    int cpt_detec {0};
    for (;;) {
        Camera.grab();
        Camera.retrieve(frame);
        if (cv::waitKey(1) > 0) {
            break;
        }
        cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // and equalize Histo (as model pictures)
        if (bHisto)equalizeHist( gray, gray);
        // detect faces
        face_cascade.detectMultiScale(gray, faces, 1.1, 3, 0|CV_HAAR_SCALE_IMAGE, cv::Size(80,80));
        if(faces.size() == 0) {
            prediction = -1;
            cpt_detec = 0;
        }
        // for each faces founded
        for(int i = 0; i < faces.size(); i++) {
            // crop face
            cv::Rect face_i = faces[i];

            face = gray(face_i);
            //  resized face and display it
            cv::resize(face, face_resized, cv::Size(IM_WIDTH, IM_HEIGHT), 1.0, 1.0, cv::INTER_CUBIC/*CV_INTER_NN*/); //INTER_CUBIC);
            if(display == 1) {
                cv::imshow("ROI", face_resized);
            }

            // now, we try to predict who is it ?
            double predicted_confidence	= 0.0;
            model->predict(face_resized,prediction,predicted_confidence);

            // create a rectangle around the face
            //rectangle(gray, face_i, CV_RGB(255, 255 ,255), 1);

            // if good prediction : > threshold
            if (predicted_confidence>PREDICTION_SEUIL) {
                // display name of the guy on the picture
                std::string box_text;
                if (prediction<people_map.size()) {
                    box_text = "Id="+people_map[prediction] + " - " + std::to_string(predicted_confidence);
                    vector_pred[cpt_detec] = prediction;
                    cpt_detec++;
                    if(cpt_detec > vector_pred.size()) {
                        cpt_detec = 0;
                        std::sort(vector_pred.begin(), vector_pred.end());
                        if(vector_pred[0] == vector_pred[vector_pred.size()-1]) {
                            switch(prediction) {
                            case 0 : {
                                mqtt_publish(&client, "Handsome boy spotted !");
                                std::thread t1(say_this, "salut, beau gosse !");
                                t1.detach();
                            }
                                break;
                            case 1 : {
                                mqtt_publish(&client, "<3");
                                std::thread t1(say_this, "c'est toi, Namour !");
                                t1.detach();
                            }
                                break;
                            case 2 : {
                                mqtt_publish(&client, "Hey Matt !");
                                std::thread t1(say_this, "Salut Matthieu !");
                                t1.detach();
                            }
                                break;
                            case 3 : {
                                mqtt_publish(&client, "Hey Boubi !");
                                std::thread t1(say_this, "il est fou ce Boubi ! !");
                                t1.detach();
                            }
                                break;
                            default :
                                break;
                            }
                            last_prediction = prediction;
                        } else {
                            last_prediction = -1;
                        }
                    }
                } else {
                    std::cout << "(E) prediction id incoherent" << std::endl;
                }
                int pos_x = std::max(face_i.tl().x - 10, 0);
                int pos_y = std::max(face_i.tl().y - 10, 0);

                cv::putText(frame, box_text, cv::Point(pos_x, pos_y), cv::FONT_HERSHEY_PLAIN, 1.0, CV_RGB(255,255,255), 1.0);
                cv::Point center(faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
                cv::ellipse(frame, center, cv::Size( faces[i].width/2, faces[i].height/2 ), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 2, 8, 0 );
            } else {
                cpt_detec = 0;
                // trace is commented to speed up
                //sprintf(sTmp,"- prediction too low = %s (%d) confiance = (%d)",people[prediction].c_str(),prediction,(int)predicted_confidence);
                //trace((string)(sTmp));
            }
        } // end for
        if(last_prediction > -1 && prediction != last_prediction) {
            last_prediction = prediction;
            mqtt_publish(&client, " ");
        }
        /////////////////////////
        // END OF FACE RECO
        /////////////////////////

        // Show the result:
        if(display == 1) {
            cv::imshow("camcvWin", frame);
            key = (char) cv::waitKey(1);
            nCount++;		// count frames displayed
        }

    }
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    std::cout << "Stopping camera.." << std::endl;
    Camera.release();
    return 0;
}
