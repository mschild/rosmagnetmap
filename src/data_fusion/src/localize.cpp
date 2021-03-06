/// \file 
 
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32MultiArray.h"
#include "geometry_msgs/Pose2D.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <sstream>
 
 // PCL 
#include <sensor_msgs/PointCloud2.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
  
#define _PI_ 3.141592654
#define _ARRAY_DIST_ 0.68
#define M_ONE -1
#define SCALED 0.92 
  
float offset[10][3] = {
    {31.0, 31.5, 81.5},
    {20.0, -5.0, 104.0},
    {34.0, -3.0, 110.0},
    {26.5, -38.5, 100.5},
    {20.0, -4.0, 100.5},
    {-17.5, -33.0, 77.0},
    {32.0, 64.0, 109.0},
    {25.0, 29.0, 93.0},
    {28.0, 37.5, 105.0},
    {29.0, 10.5, 91.0}
};
  
float MAGN_SCALE[10][3] = {
    {-6, -1.5, 0.5}, 
    {-10, -2, -2}, 
    {-6, 1, -24}, 
    {9.5, -5.5, -2.5}, 
    {17, 2, 3.5}, 
    {32.5, 35, 13}, 
    {-9, 4, 6}, 
    {4, -3, -16}, 
    {4, 4.5, 2}, 
    {9, -2.5, 12}
};
  
typedef pcl::PointCloud<pcl::PointXYZRGB> PointCloud;
PointCloud::Ptr location(new PointCloud);
  
// create a publisher for PC 
ros::Publisher pub_location_pcl;
 
std::ofstream localize("localized.txt");
///new 2 dimensional Array for the Data     
float new_data[10][3];
  
///the Array of the map for global access
float** map_arr;
 
///get the Number of Lines of the map.txt
const int get_linenumber(std::ifstream& datei){
    int i = 0;
    std::string line;
    while(!datei.eof() && std::getline(datei, line)){
        i++;
    }
    return i;
}
  
///open the file for the get_linenumber function (you must open it a second time, since it gets used in the function)
std::ifstream map("magnetmapraw.txt");

///needs to be global, because we need the line number in most of the functions
const int t = get_linenumber(map);          
  
  
///parsing the file to a 2-dimension-float-Array: [ lines (dyn) ][ Position + Sensor-data (normalized) (13) ]
void get_map_data(){
    std::ifstream map("magnetmapraw.txt");
  
    ///allocate memory for the 2-dimensional-Array
    map_arr = new float*[t];
    for (int i = 0; i < t; ++i) {
        map_arr[i] = new float[13];
    }
  
    std::string line;
  
    int count = 0;
    
    ///read line by line, each line has 11 * 3 values devided by ";", the first 3 are the position data
    while (!map.eof() && std::getline(map, line)){
  
        int actual_pos = 0;
        int pos = 0;
        int pos2 = 0;
        int pos3 = 0;
        std::string x, y, z;
  
        for (int i = 0; i < 13; ++i){
            ///get the position of the 3 next ","
            pos = line.find(",", actual_pos);
            pos2 = line.find(",", pos + 1);
            pos3 = line.find(",", pos2 + 1);
  
            if (i < 12){
            ///get the 3 next substrings from the last "," to the next ","
            x = line.substr(actual_pos, pos - (actual_pos));
            y = line.substr(pos + 1, pos2 - (pos + 1));
            z = line.substr(pos2 + 1, pos3 - (pos2 + 1));
            }
            else{
            ///for the last 3 terms, since it don't end with ","
                x = line.substr(actual_pos, pos - (actual_pos));
                y = line.substr(pos + 1, pos2 - (pos + 1));
                z = line.substr(pos2 + 1);
            }
  
            ///put the values in the array (as float), the first 3 are the position data, they have to be handeled different
            if(i >= 3){
                float norm = sqrt(pow(std::atof(x.c_str()),2)
                                +pow(std::atof(y.c_str()),2)
                                +pow(std::atof(z.c_str()),2));
                map_arr[count][i] = norm;
 
//ROS_INFO("Line: [%i]", count);
//ROS_INFO("Norm: [%f]", map_arr[count][i]);
            }else{
                map_arr[count][i] = std::atof(x.c_str());
                map_arr[count][++i] = std::atof(y.c_str());
                map_arr[count][++i] = std::atof(z.c_str());
            }
             
            actual_pos = pos3 + 1;
        }
  
        ++count;
    }
}

std::string localize_on_map(float** actual_map, float new_data[][3], float x_pos, float y_pos, float grad){
    
    ROS_INFO("locate"); 
    ///get the normalized magnet data: sqrt(x²+y²+z²)
    float norm_data[10];
    for (int i = 0; i < 10; ++i){
        norm_data[i] = sqrt(pow(new_data[i][0],2)+pow(new_data[i][1],2)+pow(new_data[i][2],2));
    }
     
     
    float  minSAD = 10000;
    int map_rows = t;
    int map_cols = 10;          ///the width of the map (actually its always 10, since the array has 10 sensors)
    int actual_rows = 1;        ///always 1
    int actual_cols = 10;       ///always 10
    float SAD = 0;
    float position[3];
 
    ///loop through the map data
    for (int x = 0; x <= map_rows - actual_rows; x++) {
        for ( int y = 0; y <= map_cols - actual_cols; y++ ) {
            SAD = 0.0;
//ROS_INFO("********************************************");
 
            ///loop through the actual data
            for ( int j = 0; j < actual_cols; j++ )
                for ( int i = 0; i < actual_rows; i++ ) {
 
                    ///first 3 pos are the position data -> y+3+j 
                    float map_norm = actual_map[x+i][y+3+j];      
                    float actual_norm = norm_data[j];
//ROS_INFO("s_norm: [%f]",S_NORM );
//ROS_INFO("t_norm: [%f]", T_NORM);
 
                     
                    SAD += abs(map_norm - actual_norm);
//ROS_INFO("SAD: [%f]", SAD);
                }
    
            ///save the best found position 
            if ( minSAD > SAD ) { 
                minSAD = SAD;
                ///give me min SAD
                position[0] = x;
                position[1] = y;
                position[2] = SAD;
            }
        }
    }
    
    //ROS_INFO("set location"); 
    location->points[0].x = actual_map[static_cast<int>(position[0])][0];
    location->points[0].y = actual_map[static_cast<int>(position[0])][1];
    location->points[0].z = 0.1;
    
    location->points[1].x = x_pos;
    location->points[1].y = y_pos;
    location->points[1].z = 0.1;
    
    pub_location_pcl.publish(location);
     
    
    ///return the position with the lowest difference to the actual magnet data
    std::stringstream pos;
    pos <<"You are here: " << "\n" << " x: "
        << actual_map[static_cast<int>(position[0])][0] << " y: "
        << actual_map[static_cast<int>(position[0])][1] << " theta: " 
        << actual_map[static_cast<int>(position[0])][2] << " SAD: "
        << position[2] << "\n";
 
  //ROS_INFO("locate finished"); 
    return pos.str();
}
   
///listens for position data 
void Callback_position(const geometry_msgs::Pose2D::ConstPtr& msg)
{
      
     
    ///Position of the middle of the Array
    ///68 cm behind the Robot, just took the 4 quadrants as direction
 
    float theta = msg->theta;
    float x_pos = msg->x;
    float y_pos = msg->y;
      
    ///get the Position of the middle of the Array
    if(theta >= _PI_/4 && theta <= _PI_*3/4){
        x_pos += _ARRAY_DIST_;                
    }
    else if(theta <= -_PI_/4 && theta >= -_PI_*3/4){
        x_pos -= _ARRAY_DIST_;                             
    }
    else if(theta < _PI_/4 && theta > -_PI_/4){
        y_pos -= _ARRAY_DIST_;             
    }
    else{
        y_pos += _ARRAY_DIST_;             
    }
      
    ROS_INFO("x: [%f]", x_pos);
    ROS_INFO("y: [%f]", y_pos);
    ROS_INFO("theta (RAD): [%f]", theta); 
    ROS_INFO("theta (Grad): [%f]", theta*(360/(2*_PI_)));
      
  
    ///get the actual position on the map
    std::string actual_pos = localize_on_map(map_arr, new_data, x_pos, y_pos, theta*(360 / (2 * _PI_)));
  
    ROS_INFO("%s",actual_pos.c_str());
    
    localize 
    << "x: " << x_pos 
    << "y: " << y_pos 
    << "theta: " << theta*(360/(2*_PI_)) << "\n"
    << "    " << actual_pos << std::endl; 
}   
  
///listens for sensorData
void Callback_data(/* datatypes */ const std_msgs::Float32MultiArray::ConstPtr& msg)
{
   
//ROS_INFO("********************************************");
    float x_avarage = 0;
    float y_avarage = 0;
    float z_avarage = 0;
      
    ///calculate the offset away and put the data in a new Array
    for(int i = 0; i < 10; i++){
        new_data[i][0] = msg->data[i*3] - offset[i][0];
        new_data[i][1] = msg->data[1+i*3] - offset[i][1];
        new_data[i][2] = msg->data[2+i*3] - offset[i][2];
          
        ///scale it 
        for(int j = 0; j < 3; ++j){
            if (new_data[i][j] < 0) {
                new_data[i][j] = (std::abs(new_data[i][j]) + MAGN_SCALE[i][j]) * M_ONE * SCALED ;
            } else {
                new_data[i][j] = (new_data[i][j] + MAGN_SCALE[i][j]) * SCALED;
            }   
              
        }
/*
ROS_INFO("Sensor: [%i]", i);
ROS_INFO("x: [%f]", new_data[i][0]);
ROS_INFO("y: [%f]", new_data[i][1]);
ROS_INFO("z: [%f]", new_data[i][2]);
ROS_INFO("********************************************");
*/
 
    }    
      
}
  
  
int main(int argc, char **argv)
{
  ROS_INFO("start listening\n");
  
  ros::init(argc, argv, "listener");
  
  ros::NodeHandle n;
  
  
  ros::Subscriber sub_pos = n.subscribe("PoseChannel", 1000, Callback_position);
  ros::Subscriber sub_data = n.subscribe("sensorData", 1000, Callback_data);
  
  ///read the map and parse it (saved in map_arr[][])
  get_map_data();
  
  pub_location_pcl =  n.advertise<PointCloud> ("location_point", 1000);
  
  location->header.frame_id = "location";
  
  location->height = 1;   // 1 for unorganized map
  location->width = 2;
  location->points.push_back (pcl::PointXYZRGB(255,0,0));
  location->points.push_back (pcl::PointXYZRGB(0,255,0));
  
  ros::spin();
  localize.close();
  
  //clear memory
  /*
 
    for (int i = 0; i < t; ++j)
          delete[] map_arr[i];
  
    delete[] map_arr;
 */
  return 0;
}
