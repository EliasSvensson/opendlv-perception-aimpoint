/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

#include<time.h>


//// parameters
//cv::Scalar const hsvBlueLow(115, 100, 30); // Note: H [0,180], S [0,255], V [0,255] (H usually defined as [0,360] otherwise)
//cv::Scalar const hsvBlueHigh(145, 255, 200);
//cv::Scalar const hsvYellowLow(10, 40, 46);
//cv::Scalar const hsvYellowHigh(34, 255, 200);
//cv::Scalar const hsvRedLow(170, 120, 100);
//cv::Scalar const hsvRedHigh(180, 180, 160);
//
//uint32_t crop_up = 430;
//uint32_t crop_down = 570;
//int32_t crop_up_min_cone_area = 600;
//int32_t crop_down_min_cone_area = 1000;
//int32_t max_unmatch_y = 30;


//check clockwise or counterclockwise
//bool check_clockwise = false;
int32_t clockwise = 0;

//aim point
float near_angle = 0.0f;
float far_angle = 0.0f;


void near_far_point(cv::Point& near0, cv::Point& near1, cv::Point& far0, cv::Point& far1, cv::Point& origin,cv::Point& offset, float& m_near_angle, float& m_far_angle){
    if(near0 == near1){
        cv::Point near_center = (near0 + near1)/2 + offset;
        cv::Point near_vector = near_center - origin;
        m_near_angle = atan2f(static_cast<float>(near_vector.x), static_cast<float>(near_vector.y)) ;
        m_far_angle = m_near_angle;
    }
    else{
        cv::Point near_center = (near0 + near1)/2 + offset;
        cv::Point near_vector = near_center - origin;
        cv::Point far_center = (far0 + far1)/2 + offset;
        cv::Point far_vector = far_center - origin;
        m_near_angle = atan2f(static_cast<float>(near_vector.x), static_cast<float>(near_vector.y)) ;
        m_far_angle = atan2f(static_cast<float>(far_vector.x), static_cast<float>(far_vector.y));
    }
}


int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --name:   name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:  width of the frame" << std::endl;
        std::cerr << "         --height: height of the frame" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=112 --name=img.argb --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};

        const uint8_t Bl0{static_cast<uint8_t>(std::stoi(commandlineArguments["Bl0"]))};
        const uint8_t Bl1{static_cast<uint8_t>(std::stoi(commandlineArguments["Bl1"]))};
        const uint8_t Bl2{static_cast<uint8_t>(std::stoi(commandlineArguments["Bl2"]))};
        const uint8_t Bh0{static_cast<uint8_t>(std::stoi(commandlineArguments["Bh0"]))};
        const uint8_t Bh1{static_cast<uint8_t>(std::stoi(commandlineArguments["Bh1"]))};
        const uint8_t Bh2{static_cast<uint8_t>(std::stoi(commandlineArguments["Bh2"]))};

        const uint8_t Yl0{static_cast<uint8_t>(std::stoi(commandlineArguments["Yl0"]))};
        const uint8_t Yl1{static_cast<uint8_t>(std::stoi(commandlineArguments["Yl1"]))};
        const uint8_t Yl2{static_cast<uint8_t>(std::stoi(commandlineArguments["Yl2"]))};
        const uint8_t Yh0{static_cast<uint8_t>(std::stoi(commandlineArguments["Yh0"]))};
        const uint8_t Yh1{static_cast<uint8_t>(std::stoi(commandlineArguments["Yh1"]))};
        const uint8_t Yh2{static_cast<uint8_t>(std::stoi(commandlineArguments["Yh2"]))};

        cv::Scalar const hsvBlueLow(Bl0, Bl1, Bl2);
        cv::Scalar const hsvBlueHigh(Bh0, Bh1, Bh2);
        cv::Scalar const hsvYellowLow(Yl0, Yl1, Yl2);
        cv::Scalar const hsvYellowHigh(Yh0, Yh1, Yh2);

        const int32_t crop_up{static_cast<int32_t>(std::stoi(commandlineArguments["crop_up"]))};
        const int32_t crop_down{static_cast<int32_t>(std::stoi(commandlineArguments["crop_down"]))};
        const int32_t crop_up_min_cone_area{static_cast<int32_t>(std::stoi(commandlineArguments["crop_up_min_cone_area"]))};
        const int32_t crop_down_min_cone_area{static_cast<int32_t>(std::stoi(commandlineArguments["crop_down_min_cone_area"]))};
        const int32_t max_unmatch_y{static_cast<int32_t>(std::stoi(commandlineArguments["max_unmatch_y"]))};
        clockwise = static_cast<int32_t>(std::stoi(commandlineArguments["clockwise"]));

        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        // Attach to the shared memory.
        std::unique_ptr<cluon::SharedMemory> sharedMemory{new cluon::SharedMemory{NAME}};
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << argv[0] << ": Attached to shared memory '" << sharedMemory->name() << " (" << sharedMemory->size() << " bytes)." << std::endl;

            // Interface to a running OpenDaVINCI session; here, you can send and receive messages.
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning()) {
                cv::Mat img;

                // Wait for a notification of a new frame.
                sharedMemory->wait();

                // Lock the shared memory.
                sharedMemory->lock();
                {
                    // Copy image into cvMat structure.
                    // Be aware of that any code between lock/unlock is blocking
                    // the camera to provide the next frame. Thus, any
                    // computationally heavy algorithms should be placed outside
                    // lock/unlock
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC4, sharedMemory->data());
                    img = wrapped.clone();
                }
                sharedMemory->unlock();

                // TODO: Do something with the frame.

                // global vars
                cv::Point vehicle_origin(WIDTH/2, HEIGHT);
                cv::Point offset(0, crop_up);

                // crop the image
                cv::Mat predict_crop_img;
                predict_crop_img = img(cv::Rect(0,crop_up,WIDTH,crop_down-crop_up));

                // detect by hsv color
                cv::Mat hsv;
                cv::cvtColor(predict_crop_img, hsv, cv::COLOR_BGR2HSV);

                // filter by hsv
                cv::Mat blueCones;
                cv::Mat yellowCones;
                cv::Mat redCones;
                cv::inRange(hsv, hsvBlueLow, hsvBlueHigh, blueCones);
                cv::inRange(hsv, hsvYellowLow, hsvYellowHigh, yellowCones);

                // Open, close and dilate
                cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3),cv::Point(-1,1));
                for (int i = 0; i < 3; i++) {
                    cv::morphologyEx(blueCones, blueCones, cv::MORPH_OPEN, kernel, cv::Point(-1,-1), 1, 1, 1); // removes noise in the background
                    cv::morphologyEx(blueCones, blueCones, cv::MORPH_CLOSE, kernel, cv::Point(-1,-1), 5, 1, 1); // removes noise in the cones
                    cv::morphologyEx(blueCones, blueCones, cv::MORPH_DILATE, kernel, cv::Point(-1, -1), 2, 1, 1); // enlarges the segmented area with cones

                    cv::morphologyEx(yellowCones, yellowCones, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1, 1, 1);
                    cv::morphologyEx(yellowCones, yellowCones, cv::MORPH_CLOSE, kernel, cv::Point(-1,-1), 5, 1, 1);
                    cv::morphologyEx(yellowCones, yellowCones, cv::MORPH_DILATE, kernel, cv::Point(-1, -1), 2, 1, 1);

                }


                int32_t mid_line = (crop_down-crop_up)/2 + crop_up;
                // find center of blue cones
                std::vector<cv::Point> positionBlueCones;
                std::vector<std::vector<cv::Point>> contoursBlue;
                std::vector<cv::Vec4i> hierarchyBlue;
                cv::findContours(blueCones, contoursBlue, hierarchyBlue, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

                for(auto & i : contoursBlue) {
                    cv::RotatedRect rectRotated = cv::minAreaRect(i);
                    cv::Rect rect = rectRotated.boundingRect();
                    if ((rect.area() > crop_up_min_cone_area && rect.tl().y < mid_line) || (rect.area() > crop_down_min_cone_area && rect.tl().y > mid_line)) {
                        if(rect.height>0.9*rect.width){
                            cv::Point cone_center = rect.tl()+cv::Point(rect.width/2, rect.height/2);
                            positionBlueCones.push_back(cone_center);
                        }

                    }
                }

                // find center of yellow cones
                std::vector<cv::Point> positionYellowCones;
                std::vector<std::vector<cv::Point>> contoursYellow;
                std::vector<cv::Vec4i> hierarchyYellow;
                cv::findContours(yellowCones, contoursYellow, hierarchyYellow, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
                for(auto & i : contoursYellow) {
                    cv::RotatedRect rectRotated = cv::minAreaRect(i);
                    cv::Rect rect = rectRotated.boundingRect();
                    if ((rect.area() > crop_up_min_cone_area && rect.tl().y < mid_line) || (rect.area() > crop_down_min_cone_area && rect.tl().y > mid_line)) {
                        if(rect.height>0.9*rect.width){
                            cv::Point cone_center = rect.tl()+cv::Point(rect.width/2, rect.height/2);
                            positionYellowCones.push_back(cone_center);
                        }
                    }
                }


                // remove the unmatch point
                if(positionBlueCones.size()>0 && positionYellowCones.size()>0){
                    if(positionBlueCones[0].y-positionYellowCones[0].y>max_unmatch_y){
                        positionBlueCones.erase(positionBlueCones.begin());
                    }
                    if(positionYellowCones[0].y-positionBlueCones[0].y>max_unmatch_y){
                        positionYellowCones.erase(positionYellowCones.begin());
                    }
                }


                //steering
                if(positionYellowCones.size()==0 || positionBlueCones.size()==0){
                    //counterclockwise turn left
                    if(positionBlueCones.size()>=2 && positionYellowCones.size()==0){
                        if(clockwise){
                            cv::Point right_down_corner(WIDTH,crop_down-crop_up);
                            near_far_point(positionBlueCones[0], right_down_corner, positionBlueCones[1], right_down_corner, vehicle_origin, offset, near_angle, far_angle);
                        }
                        else{
                            cv::Point left_down_corner(0,crop_down-crop_up);
                            near_far_point(positionBlueCones[0], left_down_corner, positionBlueCones[1], left_down_corner, vehicle_origin, offset, near_angle, far_angle);
                        }
                    }

                    //counterclockwise turn right
                    if(positionYellowCones.size()>=2 && positionBlueCones.size()==0){
                        if(clockwise){
                            cv::Point left_down_corner(0,crop_down-crop_up);
                            near_far_point(positionYellowCones[0], left_down_corner, positionYellowCones[1], left_down_corner, vehicle_origin, offset, near_angle, far_angle);
                        }
                        else{
                            cv::Point right_down_corner(WIDTH,crop_down-crop_up);
                            near_far_point(positionYellowCones[0], right_down_corner, positionYellowCones[1], right_down_corner, vehicle_origin, offset, near_angle, far_angle);
                        }
                    }
                }


                // go straight
                if(positionYellowCones.size()>0 && positionBlueCones.size()>0){
                    if(positionYellowCones.size()>=2 && positionBlueCones.size()>=2){
                        near_far_point(positionYellowCones[0], positionBlueCones[0], positionYellowCones[1], positionBlueCones[1], vehicle_origin, offset,  near_angle, far_angle);
                    }

                    if((positionYellowCones.size()==1 && positionBlueCones.size()>=2) || (positionBlueCones.size()==1 && positionYellowCones.size()>=2)){
                        near_far_point(positionYellowCones[0], positionBlueCones[0], positionYellowCones[0], positionBlueCones[0], vehicle_origin, offset,  near_angle, far_angle);
                    }
                }


                if (VERBOSE) {
                    //connecting the cones
                    cv::line(img, offset, cv::Point(WIDTH,crop_up),cv::Scalar(100, 0, 100));
                    cv::line(img, cv::Point(0,crop_down), cv::Point(WIDTH,crop_down),cv::Scalar(100, 0, 100));
                    if(positionBlueCones.size()>=2){
                        for( uint32_t i = 0; i < positionBlueCones.size()-1; i++) {
                            cv::Point delta = positionBlueCones[i+1] - positionBlueCones[i];
                            if( norm(delta)< WIDTH/4 ){
                                cv::line(img, positionBlueCones[i]+offset, positionBlueCones[i+1]+offset,cv::Scalar(255, 0, 255));
                            }
                        }
                    }
                    if(positionYellowCones.size()>=2){
                        for( uint32_t i = 0; i < positionYellowCones.size()-1; i++) {
                            cv::Point delta = positionYellowCones[i+1] - positionYellowCones[i];
                            if( norm(delta)< WIDTH/4 ){
                                cv::line(img, positionYellowCones[i]+offset, positionYellowCones[i+1]+offset,cv::Scalar(255, 0, 255));
                            }
                        }
                    }
                    // add perception mask
                    cv::Mat blue_mask;
                    cv::Mat blue_mask_color;
                    cv::copyMakeBorder(blueCones, blue_mask, crop_up,HEIGHT-crop_down,0,0,cv::BORDER_CONSTANT, cv::Scalar(0,0,0));
                    cv::cvtColor(blue_mask, blue_mask_color, cv::COLOR_GRAY2RGBA);
                    cv::addWeighted(img, 1.0, blue_mask_color, 0.2, 0.0, img);

                    cv::Mat yellow_mask;
                    cv::Mat yellow_mask_color;
                    cv::copyMakeBorder(yellowCones, yellow_mask, crop_up,HEIGHT-crop_down,0,0,cv::BORDER_CONSTANT, cv::Scalar(0,0,0));
                    cv::cvtColor(yellow_mask, yellow_mask_color, cv::COLOR_GRAY2RGBA);
                    cv::addWeighted(img, 1.0, yellow_mask_color, 0.2, 0.0, img);

                    cv::imshow(sharedMemory->name().c_str(), img);

                    cv::waitKey(1);
                }


                ////////////////////////////////////////////////////////////////
                opendlv::logic::action::AimPoint nearAim;
                nearAim.azimuthAngle(near_angle);
                nearAim.zenithAngle(0);
                nearAim.distance(0);
                cluon::data::TimeStamp t1 = cluon::time::now();
                od4.send(nearAim, t1, 0);

                opendlv::logic::action::AimPoint farAim;
                farAim.azimuthAngle(far_angle);
                farAim.zenithAngle(0);
                farAim.distance(0);
                cluon::data::TimeStamp t2 = cluon::time::now();
                od4.send(farAim, t2, 1);
            }
        }
        retCode = 0;
    }
    return retCode;
}

