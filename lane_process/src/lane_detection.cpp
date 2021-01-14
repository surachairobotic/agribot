#include <chrono>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/point_cloud_conversion.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>

using std::placeholders::_1;

class LaneDetection : public rclcpp::Node
{
public:
  LaneDetection()
  : Node("pc_sub"), b_run(false)
  {
    this->pc = new sensor_msgs::msg::PointCloud();
    this->pc2 = new sensor_msgs::msg::PointCloud2();
    this->quat2base = new tf2::Quaternion(-0.6123724,0.6123724,-0.3535534,0.3535534);
    this->matrix2base = new tf2::Matrix3x3(*quat2base);
    this->matrix2base = new tf2::Matrix3x3(this->matrix2base->inverse());
    publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud>("/agribot/pc", 10);
    subscription_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      "/agribot/camera/rs_front/points", 10, std::bind(&LaneDetection::pc_cb, this, _1));
    timer_ = this->create_wall_timer(std::chrono::milliseconds(50), 
                                     std::bind(&LaneDetection::timer_callback, this));
  }

private:
  void pc_cb(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
  {
    RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg->header.frame_id.c_str());
    if(!b_run) {
      this->pc2->header = msg->header;
      this->pc2->height = msg->height;
      this->pc2->width = msg->width;
      this->pc2->fields = msg->fields;
      this->pc2->is_bigendian = msg->is_bigendian;
      this->pc2->point_step = msg->point_step;
      this->pc2->row_step = msg->row_step;
      this->pc2->data = msg->data;
      this->pc2->is_dense = msg->is_dense;
      b_run = true;
    }
  }
  
  void timer_callback() {
    RCLCPP_INFO(this->get_logger(), "timer : start");
    RCLCPP_INFO(this->get_logger(), "%lf, %lf, %lf", this->matrix2base->getRow(0).x(), this->matrix2base->getRow(0).y(), this->matrix2base->getRow(0).z());
    RCLCPP_INFO(this->get_logger(), "%lf, %lf, %lf", this->matrix2base->getRow(1).x(), this->matrix2base->getRow(1).y(), this->matrix2base->getRow(1).z());
    RCLCPP_INFO(this->get_logger(), "%lf, %lf, %lf", this->matrix2base->getRow(2).x(), this->matrix2base->getRow(2).y(), this->matrix2base->getRow(2).z());
    RCLCPP_INFO(this->get_logger(), "timer : end");
    if(b_run) {
      pc2_to_pc();
      b_run = false;
    }
  }
  
  void pc2_to_pc(void) {
    RCLCPP_INFO(this->get_logger(), "pc2_to_pc : start");
    RCLCPP_INFO(this->get_logger(), "frame_id : %s", pc2->header.frame_id.c_str());    
    sensor_msgs::convertPointCloud2ToPointCloud(*pc2, *pc);
    RCLCPP_INFO(this->get_logger(), "frame_id : %s", pc->header.frame_id.c_str());    
    double x[2]={9999.99, 0.0}, y[2]={9999.99, 0.0}, z[2]={9999.99, 0.0}, _x, _y, _z;
    for(unsigned int i=0; i<pc->points.size(); ++i) {
      pc->points[i].x -= 0.45;
      pc->points[i].z -= 1.5;
      _x = pc->points[i].x;
      _y = pc->points[i].y;
      _z = pc->points[i].z;
      pc->points[i].x = (_x * this->inv2base->getRow(0).x()) + 
                        (_y * this->inv2base->getRow(0).y()) +
                        (_z * this->inv2base->getRow(0).z());// + 0.45;
      pc->points[i].y = (_x * this->inv2base->getRow(1).x()) + 
                        (_y * this->inv2base->getRow(1).y()) +
                        (_z * this->inv2base->getRow(1).z());
      pc->points[i].z = (_x * this->inv2base->getRow(2).x()) + 
                        (_y * this->inv2base->getRow(2).y()) +
                        (_z * this->inv2base->getRow(2).z());// + 1.5;
      if(pc->points[i].x<x[0])
        x[0]=pc->points[i].x;
      if(pc->points[i].y<y[0])
        y[0]=pc->points[i].y;
      if(pc->points[i].z<z[0])
        z[0]=pc->points[i].z;
      if(pc->points[i].x>x[1])
        x[1]=pc->points[i].x;
      if(pc->points[i].y>y[1])
        y[1]=pc->points[i].y;
      if(pc->points[i].z>z[1])
        z[1]=pc->points[i].z;
    }
    RCLCPP_INFO(this->get_logger(), "%lf, %lf, %lf : %lf, %lf, %lf", x[0], y[0], z[0], x[1], y[1], z[1]);    
    //RCLCPP_INFO(this->get_logger(), "pc2_to_pc : start");
    publisher_->publish(*pc);
    //RCLCPP_INFO(this->get_logger(), "pc2_to_pc : finished");
  }

  rclcpp::Publisher<sensor_msgs::msg::PointCloud>::SharedPtr publisher_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr timer_;

  sensor_msgs::msg::PointCloud2 *pc2;
  sensor_msgs::msg::PointCloud *pc;
  tf2::Quaternion *quat2base;
  tf2::Matrix3x3 *matrix2base;
  tf2::Matrix3x3 *inv2base;
  bool b_run;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<LaneDetection>());
  rclcpp::shutdown();
  return 0;
}
