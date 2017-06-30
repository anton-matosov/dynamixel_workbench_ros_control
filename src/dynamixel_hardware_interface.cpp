#include <dynamixel_workbench_ros_control/dynamixel_hardware_interface.h>

namespace dynamixel_workbench_ros_control {


DynamixelHardwareInterface::DynamixelHardwareInterface() {



}

bool DynamixelHardwareInterface::init() {
  // Load dynamixel config from parameter server
  ros::NodeHandle pnh("~");
  if (!loadDynamixels(pnh)) {
    ROS_ERROR_STREAM("Failed to ping all motors.");
    return false;
  }
  // initialize sync read/write
  driver_->initSyncRead();
  driver_->initSyncWrite();

  joint_count_ = joint_names_.size();
  current_position_.resize(joint_count_, 0);
  current_velocity_.resize(joint_count_, 0);
  current_effort_.resize(joint_count_, 0);
  goal_position_.resize(joint_count_, 0);
  goal_velocity_.resize(joint_count_, 0);
  goal_effort_.resize(joint_count_, 0);

  // register interfaces
  // connect and register the joint state interface
  for (unsigned int i = 0; i < joint_names_.size(); i++) {
    hardware_interface::JointStateHandle state_handle(joint_names_[i], &current_position_[i], &current_velocity_[i], &current_effort_[i]);
    jnt_state_interface.registerHandle(state_handle);

    hardware_interface::JointHandle pos_handle(state_handle, &goal_position_[i]);
    jnt_pos_interface.registerHandle(pos_handle);
  }
  registerInterface(&jnt_state_interface);
  registerInterface(&jnt_pos_interface);

  setTorque(true);

//  transmission_interface::ActuatorData a_data;
//  a_data.position.push_back(&a_pos);

//  transmission_interface::JointData j_data;
//  j_data.position.push_back(&j_pos);

//  // Transmission interface
//  transmission_interface::ActuatorToJointStateInterface act_to_jnt_pos;
//  act_to_jnt_pos.registerHandle(ActuatorToJointPositionHandle("trans", &trans, a_data, j_data));

//  // Propagate actuator position to joint space
//  act_to_jnt_pos.propagate();
}

bool DynamixelHardwareInterface::loadDynamixels(ros::NodeHandle& nh) {
  ROS_INFO_STREAM("Loading parameters from namespace " << nh.getNamespace() + "/dynamixels");

  // get port info
  std::string port_name;
  nh.getParam("dynamixels/port_info/port_name", port_name);
  int baudrate;
  nh.getParam("dynamixels/port_info/baudrate", baudrate);
  float protocol_version;
  nh.getParam("dynamixels/port_info/protocol_version", protocol_version);
  driver_.reset(new dynamixel_multi_driver::DynamixelMultiDriver(port_name, baudrate, protocol_version));

  // get dxl info
  std::vector<dynamixel_driver::DynamixelInfo*> infos;
  XmlRpc::XmlRpcValue dxls;
  nh.getParam("dynamixels/device_info", dxls);
  ROS_ASSERT(dxls.getType() == XmlRpc::XmlRpcValue::TypeStruct);
  for(XmlRpc::XmlRpcValue::ValueStruct::const_iterator it = dxls.begin(); it != dxls.end(); ++it) {
    std::string dxl_name = (std::string)(it->first);
    joint_names_.push_back(dxl_name);
    ros::NodeHandle dxl_nh(nh, "dynamixels/device_info/" + dxl_name);

    dynamixel_driver::DynamixelInfo * info = new dynamixel_driver::DynamixelInfo;
    int model_id;
    dxl_nh.getParam("id", model_id);
    info->model_id = model_id;
    int model_number;
    dxl_nh.getParam("model_number", model_number);
    info->model_number = model_number;
    infos.push_back(info);
  }

  // load into driver and clean up
  driver_->loadDynamixel(infos);
  for (unsigned int i = 0; i < infos.size(); i++) {
    delete infos[i];
  }
}

void DynamixelHardwareInterface::setTorque(bool enabled) {
  std::vector<uint8_t> torque(joint_names_.size(), enabled);
  driver_->syncWriteTorque(torque);
}

void DynamixelHardwareInterface::read() {
  driver_->syncReadPosition(current_position_);
}

void DynamixelHardwareInterface::write() {
  driver_->syncWritePosition(goal_position_);
}

}