#ifndef _COMMANDE_ROUES_PLUGIN_HH_
#define _COMMANDE_ROUES_PLUGIN_HH_

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <thread>
#include "ros/ros.h"
#include "ros/callback_queue.h"
#include "ros/subscribe_options.h"
#include "std_msgs/Float64.h"

namespace gazebo
{
  /// \brief A plugin to control wheels.
  class CommandeRouesFollowerPlugin : public ModelPlugin
  {
    /// \brief Constructor
    public: CommandeRouesFollowerPlugin() {}

    public: virtual void Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
    {

			// Safety check
      if (_model->GetJointCount() == 0)
      {
        std::cerr << "Invalid joint count, plugin not loaded\n";
        return;
      }

      // Store the model pointer for convenience.
      this->model = _model;

      // Create the node
      this->node = transport::NodePtr(new transport::Node());

      this->node->Init(this->model->GetWorld()->GetName());
      
      // Create a topic name
      std::string topicName = "/velCmd_topic";

			//std::cerr << "\nThe wheel command follower plugin is attach to model[" <<
        //_model->GetName() << "]\n";

      // Subscribe to the topic, and register a callback
      this->sub = this->node->Subscribe(topicName,
         &CommandeRouesFollowerPlugin::OnMsg, this);

			// Initialize ros, if it has not already bee initialized.
			if (!ros::isInitialized())
			{
				int argc = 0;
				char **argv = NULL;
				ros::init(argc, argv, "gazebo_client",
						ros::init_options::NoSigintHandler);
			}

			// Create our ROS node. This acts in a similar manner to
			// the Gazebo node
			this->rosNode.reset(new ros::NodeHandle("gazebo_client"));

			// Create a named topic, and subscribe to it.
			ros::SubscribeOptions so =
				ros::SubscribeOptions::create<std_msgs::Float64>(
						topicName,
						1,
						boost::bind(&CommandeRouesFollowerPlugin::OnRosMsg, this, _1),
						ros::VoidPtr(), &this->rosQueue);
			this->rosSub = this->rosNode->subscribe(so);

			// Spin up the queue helper thread.
			this->rosQueueThread =
				std::thread(std::bind(&CommandeRouesFollowerPlugin::QueueThread, this));
    }


		/// \brief Handle an incoming message from ROS
		/// \param[in] _msg A float value that is used to set the velocity
		public: void OnRosMsg(const std_msgs::Float64ConstPtr &_msg)
		{		
		double speed = this->model->GetRelativeLinearVel().x; // Penser a recuperer vraie valeur vitesse a partir du topic velMeasure_topic.. Pas tres pertinent.
		double deltaSpeed;
		double speedCmd;
		double threshold = 0.001;
		deltaSpeed = (-1)*speed + _msg->data;
		
		speedCmd = _msg->data;

		if (deltaSpeed > threshold)
		{
			speedCmd = speed + threshold;
		}

		if (deltaSpeed < threshold)
		{
			speedCmd = speed - (threshold+0.005);
		}

		if (speedCmd <0)
		{
			speedCmd = 0;	
		}		

		this->model->SetLinearVel(math::Vector3(speedCmd, 0, 0));

		}

		/// \brief ROS helper function that processes messages
		private: void QueueThread()
		{
			static const double timeout = 0.01;
			while (this->rosNode->ok())
			{
				this->rosQueue.callAvailable(ros::WallDuration(timeout));
			}
		}

    /// \brief Handle incoming message
    /// \param[in] _msg Repurpose a vector3 message. This function will
    /// only use the x component.
    private: void OnMsg(ConstVector3dPtr &_msg)
    {
			this->model->SetLinearVel(math::Vector3(_msg->x(), 0, 0));
    }

    /// \brief A node used for transport
    private: transport::NodePtr node;

    /// \brief A subscriber to a named topic.
    private: transport::SubscriberPtr sub;

    /// \brief Pointer to the model.
    private: physics::ModelPtr model;

		/// \brief A node use for ROS transport
		private: std::unique_ptr<ros::NodeHandle> rosNode;

		/// \brief A ROS subscriber
		private: ros::Subscriber rosSub;

		/// \brief A ROS callbackqueue that helps process messages
		private: ros::CallbackQueue rosQueue;

		/// \brief A thread the keeps running the rosQueue
		private: std::thread rosQueueThread;

  };

  // Tell Gazebo about this plugin, so that Gazebo can call Load on this plugin.
  GZ_REGISTER_MODEL_PLUGIN(CommandeRouesFollowerPlugin)
}
#endif
