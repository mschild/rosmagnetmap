#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32MultiArray.h"
#include "geometry_msgs/Pose2D.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

// PCL 
#include <sensor_msgs/PointCloud2.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
 
 #include <pcl/io/pcd_io.h>   // for saving data 
  
 
typedef pcl::PointCloud<pcl::PointXYZRGB> PointCloud;

#define _PI_ 3.141592654
#define _ARRAY_DIST_ 0.68
#define M_ONE -1.0
#define SCALED 0.92
 
#define PC_MAP_FILENAME "map.pcd"

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
 
///to syncronize position and magnet (it happened, that there was posted a line with only zeroes as magnet data)
bool magnetoms_called = false;
 
///new 2 dimensional Array for the Data     
float new_data[10][3];
 
///open file to put the mapdata in (nice to have a look)
std::ofstream Magnetmap("magnetmap.txt");
 
///open file to put the mapdata in (just the data, better to work with)
std::ofstream Magnetmapraw("magnetmapraw.txt");
 
// create a new global pointcloud (a small one)
PointCloud::Ptr PC(new PointCloud); 

// create another global pointcloud (a big one that contains all sensor values ever recorded)
PointCloud::Ptr PC_map(new PointCloud); 

// create a publisher for PC 
ros::Publisher pub_pcl; 

/// convert a MagnData array to rgb-color 
uint32_t  MagnData_to_hexcolor(float singleMagnData[3])
{
  ROS_INFO("converting...");
  uint8_t r,g,b; 
  
  if(  singleMagnData[0] > 1225 ) r = 255;
  else if (singleMagnData[0] < -1225) r = 0;
  else r = 128 + singleMagnData[0] / 10; 
  
  if(  singleMagnData[1] > 1225 ) g = 255;
  else if (singleMagnData[1] < -1225) g = 0;
  else g = 128 + singleMagnData[1] / 10;
  
  if(  singleMagnData[2] > 1225 ) b = 255;
  else if (singleMagnData[2] < -1225) b = 0;
  else b = 128 + singleMagnData[2] / 10;
  

  uint32_t rgb = ((uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b);
  return rgb; 
}



///puts all data to pointcloud
void put_data_to_PC(float MagnData[][3], float x_pos, float y_pos, float theta){

     
    ///normalize
   /* float norm_data[10];
    for (int i = 0; i < 10; ++i){
        norm_data[i] = sqrt(pow(MagnData[i][0], 2) + pow(MagnData[i][1], 2) + pow(MagnData[i][2], 2));
    }
    */
    
    ///moving towards - x (which means moving toward -y in rviz)
    if (theta >= _PI_ / 4 && theta <= _PI_ * 3 / 4){
      for( int i = 0; i < 10; i++)
      {
          PC->points[i].x = x_pos - 0.225 + i*0.05;
          PC->points[i].y = y_pos; 
          PC->points[i].z = 0; // norm_data[i]; 
          
          uint32_t rgb = MagnData_to_hexcolor(MagnData[i]); 
          PC->points[i].rgb = *reinterpret_cast<float*>( &rgb );
      }  
    }
    ///moving towards + x (which means moving toward +y in rviz)
    else if (theta <= -_PI_ / 4 && theta >= -_PI_ * 3 / 4){
      for( int i = 0; i < 10; i++)
      {
          PC->points[i].x = x_pos + 0.225 - i*0.05;
          PC->points[i].y = y_pos;
          PC->points[i].z = 0; //norm_data[i]; 
          
          uint32_t rgb = MagnData_to_hexcolor(MagnData[i]); 
          PC->points[i].rgb = *reinterpret_cast<float*>( &rgb );
      }  
    }
    ///moving towards +y (which means moving toward +x in rviz)
    else if (theta < _PI_ / 4 && theta > -_PI_ / 4){
      for( int i = 0; i < 10; i++)
      {
          PC->points[i].x = x_pos;
          PC->points[i].y = (y_pos - 0.225 + i*0.05); 
          PC->points[i].z = 0; //(int)norm_data[i] % 5; 
          
          /* 
          // pack r/g/b into rgb
          uint8_t r = (int) MagnData[i][0] % 255; 
          uint8_t g = (int) MagnData[i][1] % 255;
          uint8_t b = (int) MagnData[i][2] % 255;   
          
          uint32_t rgb = ((uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b);
          */ 
          
          // set color 
          
          uint32_t rgb = MagnData_to_hexcolor(MagnData[i]); 
          PC->points[i].rgb = *reinterpret_cast<float*>( &rgb );
      }  
    }
    ///moving towards -y (which means moving toward - x in rviz)
    else{
      for( int i = 0; i < 10; i++)
      {
          PC->points[i].x = x_pos;
          PC->points[i].y = y_pos + 0.225 - i*0.05; 
          PC->points[i].z = 0; //norm_data[i]; 
          
          uint32_t rgb = MagnData_to_hexcolor(MagnData[i]); 
          PC->points[i].rgb = *reinterpret_cast<float*>( &rgb );
      } 
    }
    
    // publish PC 
    PC->header.stamp = ros::Time::now().toNSec();
    
    // adjust PC size 
    PC_map->width += 10;
    
    // copy data to PC_map
    for( int i = 0; i < 10; i++)
        PC_map->points.push_back(PC->points[i]); 

    
    
    
    
    pub_pcl.publish(PC); 
    
}


///puts all data to one position: in the magnetmap.txt -> nicely formated
void output_mapdata (float MagnData[][3], float x_pos, float y_pos, float theta){
 
    Magnetmap         
        << "Position Data:" << std::endl 
        << " (" << x_pos <<"/ "<< y_pos <<") "<< theta << "°" << std::endl
        << "Magnet Data:" << std::endl;
 
    for(int i = 0; i < 10; i++){
            Magnetmap
                << "Sensor " << i << std::endl 
                << " x: " << MagnData[i][0] 
                <<", y: " << MagnData[i][1] 
                <<", z: " << MagnData[i][2]
                << std::endl; 
    
    }
    Magnetmap << " " << std::endl;
}
 
///puts all data to one position: in the magnetmapraw.txt -> just to parse, don't try to read
void output_mapdata_raw(float MagnData[][3], float x_pos, float y_pos, float theta){
     
 
    ///all values are seperated by ",", the first 3 are the position data
    Magnetmapraw     
        << x_pos << "," << y_pos << "," << theta;// <<",";
         
    for(int i = 0; i < 10; i++){
                       
        Magnetmapraw 
            << "," << MagnData[i][0] 
            << "," << MagnData[i][1] 
            << "," << MagnData[i][2];
            //<< ",";    
    }
    Magnetmapraw << std::endl;
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
    ///output
    ROS_INFO("x: [%f]", x_pos);
    ROS_INFO("y: [%f]", y_pos);
    ROS_INFO("theta (RAD): [%f]", theta); 
    ROS_INFO("theta (Grad): [%f]", theta*(360/(2*_PI_)));
     
    ///write the position data + magnet data to .txt files
    if (magnetoms_called == true){
        output_mapdata(new_data, x_pos, y_pos, theta*(360 / (2 * _PI_)));
        output_mapdata_raw(new_data, x_pos, y_pos, theta*(360 / (2 * _PI_)));
        
        put_data_to_PC(new_data, x_pos, y_pos, theta);
        
    }
}   
 
///listens for sensorData
void Callback_data(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
    ///to synchronize
    magnetoms_called = true;
 
    ROS_INFO("********************************************");
    float x_avarage = 0;
    float y_avarage = 0;
    float z_avarage = 0;
     
    ///calculate the offset away and put the data in a new Array
    for(int i = 0; i < 10; i++){
        new_data[i][0] = msg->data[i*3] - offset[i][0];
        new_data[i][1] = msg->data[1+i*3] - offset[i][1];
        new_data[i][2] = msg->data[2+i*3] - offset[i][2];
         
        ///scale the data
        for(int j = 0; j < 3; ++j){
            if (new_data[i][j] < 0) {
                new_data[i][j] = (std::abs(new_data[i][j]) + MAGN_SCALE[i][j]) * M_ONE * SCALED ;
            } else {
                new_data[i][j] = (new_data[i][j] + MAGN_SCALE[i][j]) * SCALED;
            }
             
  
             
        }
        ///output
        ROS_INFO("Sensor: [%i]", i);
        ROS_INFO("x: [%f]", new_data[i][0]);
        ROS_INFO("y: [%f]", new_data[i][1]);
        ROS_INFO("z: [%f]", new_data[i][2]);
    }
 
    ROS_INFO("********************************************");
     
}
 
 
int main(int argc, char **argv)
{    
  ROS_INFO("start listening\n");
  /**
   * The ros::init() function needs to see argc and argv so that it can perform
   * any ROS arguments and name remapping that were provided at the command line.
   * For programmatic remappings you can use a different version of init() which takes
   * remappings directly, but for most command-line programs, passing argc and argv is
   * the easiest way to do it.  The third argument to init() is the name of the node.
   *
   * You must call one of the versions of ros::init() before using any other
   * part of the ROS system.
   */
  ros::init(argc, argv, "listener");
 
  /**
   * NodeHandle is the main access point to communications with the ROS system.
   * The first NodeHandle constructed will fully initialize this node, and the last
   * NodeHandle destructed will close down the node.
   */
  ros::NodeHandle n;
  /**
   * The subscribe() call is how you tell ROS that you want to receive messages
   * on a given topic.  This invokes a call to the ROS
   * master node, which keeps a registry of who is publishing and who
   * is subscribing.  Messages are passed to a callback function, here
   * called chatterCallback.  subscribe() returns a Subscriber object that you
   * must hold on to until you want to unsubscribe.  When all copies of the Subscriber
   * object go out of scope, this callback will automatically be unsubscribed from
   * this topic.
   *
   * The second parameter to the subscribe() function is the size of the message
   * queue.  If messages are arriving faster than they are being processed, this
   * is the number of messages that will be buffered up before beginning to throw
   * away the oldest ones.
   */
  ros::Subscriber sub_pos = n.subscribe("PoseChannel", 1000, Callback_position);
  ros::Subscriber sub_data = n.subscribe("sensorData", 1000, Callback_data);
  
  // prepare the small PointCloud 
  
  // init publisher 
  pub_pcl =  n.advertise<PointCloud> ("points2", 1000);
  
  // give it a name 
  PC->header.frame_id = "data";
  
  // set the clouds size 
  PC->height = 1;   // 1 for unorganized map
  PC->width = 10;
  
  // push 10 points to the cloud so they can be edited later 
  for ( int i = 0; i < 10; i++)
    PC->points.push_back (pcl::PointXYZRGB(255,255,255));
  
    
  // prepare the big pointCloud
  PC_map->header.frame_id = "map";
   
  // set the clouds size (will be changed later)
  PC_map->height = 1;   // 1 for unorganized map
  PC_map->width = 0;
  
  
  
  /**
   * ros::spin() will enter a loop, pumping callbacks.  With this version, all
   * callbacks will be called from within this thread (the main one).  ros::spin()
   * will exit when Ctrl-C is pressed, or the node is shutdown by the master.
   */
  
  ros::spin();
  
  // save the big pointcloud to a file 
  pcl::io::savePCDFile (PC_MAP_FILENAME, *PC_map, false);
  
  
  /* 
  ros::Rate loop_rate(4);
  while (n.ok())
  {
    PC->header.stamp = ros::Time::now().toNSec();
    pub_pcl.publish (PC);
    ros::spinOnce ();
    loop_rate.sleep ();
  } 
  */
  

  //close the both output Files
  Magnetmap.close();
  Magnetmapraw.close();
   
  return 0;
}
