/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2012, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: sac_segmentation.h 4595 2012-02-21 07:19:46Z rusu $
 *
 */

#ifndef PCL_SEGMENTATION_SAC_SEGMENTATION_H_
#define PCL_SEGMENTATION_SAC_SEGMENTATION_H_

#include "pcl/pcl_base.h"
#include "pcl/PointIndices.h"
#include "pcl/ModelCoefficients.h"

// Sample Consensus methods
#include "pcl/sample_consensus/method_types.h"
#include "pcl/sample_consensus/sac.h"
// Sample Consensus models
#include "pcl/sample_consensus/model_types.h"
#include "pcl/sample_consensus/sac_model.h"

namespace pcl
{
  /** \brief @b SACSegmentation represents the Nodelet segmentation class for
    * Sample Consensus methods and models, in the sense that it just creates a
    * Nodelet wrapper for generic-purpose SAC-based segmentation.
    * \author Radu Bogdan Rusu
    * \ingroup segmentation
    */
  template <typename PointT>
  class SACSegmentation : public PCLBase<PointT>
  {
    using PCLBase<PointT>::initCompute;
    using PCLBase<PointT>::deinitCompute;

     public:
      using PCLBase<PointT>::input_;
      using PCLBase<PointT>::indices_;

      typedef pcl::PointCloud<PointT> PointCloud;
      typedef typename PointCloud::Ptr PointCloudPtr;
      typedef typename PointCloud::ConstPtr PointCloudConstPtr;

      typedef typename SampleConsensus<PointT>::Ptr SampleConsensusPtr;
      typedef typename SampleConsensusModel<PointT>::Ptr SampleConsensusModelPtr;

      /** \brief Empty constructor. */
      SACSegmentation () :  model_type_ (-1), method_type_ (0), optimize_coefficients_ (true), 
                            radius_min_ (-DBL_MAX), radius_max_ (DBL_MAX), eps_angle_ (0.0),
                            max_iterations_ (50), probability_ (0.99)
      {
        axis_.setZero ();
        //srand ((unsigned)time (0)); // set a random seed
      }

      /** \brief Empty destructor. */
      virtual ~SACSegmentation () { /*srv_.reset ();*/ };

      /** \brief The type of model to use (user given parameter).
        * \param[in] model the model type (check \a model_types.h)
        */
      inline void 
      setModelType (int model) { model_type_ = model; }

      /** \brief Get the type of SAC model used. */
      inline int 
      getModelType () const { return (model_type_); }

      /** \brief Get a pointer to the SAC method used. */
      inline SampleConsensusPtr 
      getMethod () const { return (sac_); }

      /** \brief Get a pointer to the SAC model used. */
      inline SampleConsensusModelPtr 
      getModel () const { return (model_); }

      /** \brief The type of sample consensus method to use (user given parameter).
        * \param[in] method the method type (check \a method_types.h)
        */
      inline void 
      setMethodType (int method) { method_type_ = method; }

      /** \brief Get the type of sample consensus method used. */
      inline int 
      getMethodType () const { return (method_type_); }

      /** \brief Distance to the model threshold (user given parameter).
        * \param[in] threshold the distance threshold to use
        */
      inline void 
      setDistanceThreshold (double threshold) { threshold_ = threshold; }

      /** \brief Get the distance to the model threshold. */
      inline double 
      getDistanceThreshold () const { return (threshold_); }

      /** \brief Set the maximum number of iterations before giving up.
        * \param[in] max_iterations the maximum number of iterations the sample consensus method will run
        */
      inline void 
      setMaxIterations (int max_iterations) { max_iterations_ = max_iterations; }

      /** \brief Get maximum number of iterations before giving up. */
      inline int 
      getMaxIterations () const { return (max_iterations_); }

      /** \brief Set the probability of choosing at least one sample free from outliers.
        * \param[in] probability the model fitting probability
        */
      inline void 
      setProbability (double probability) { probability_ = probability; }

      /** \brief Get the probability of choosing at least one sample free from outliers. */
      inline double 
      getProbability () const { return (probability_); }

      /** \brief Set to true if a coefficient refinement is required.
        * \param[in] optimize true for enabling model coefficient refinement, false otherwise
        */
      inline void 
      setOptimizeCoefficients (bool optimize) { optimize_coefficients_ = optimize; }

      /** \brief Get the coefficient refinement internal flag. */
      inline bool 
      getOptimizeCoefficients () const { return (optimize_coefficients_); }

      /** \brief Set the minimum and maximum allowable radius limits for the model (applicable to models that estimate
        * a radius)
        * \param[in] min_radius the minimum radius model
        * \param[in] max_radius the maximum radius model
        */
      inline void
      setRadiusLimits (const double &min_radius, const double &max_radius)
      {
        radius_min_ = min_radius;
        radius_max_ = max_radius;
      }

      /** \brief Get the minimum and maximum allowable radius limits for the model as set by the user.
        * \param[out] min_radius the resultant minimum radius model
        * \param[out] max_radius the resultant maximum radius model
        */
      inline void
      getRadiusLimits (double &min_radius, double &max_radius)
      {
        min_radius = radius_min_;
        max_radius = radius_max_;
      }

      /** \brief Set the axis along which we need to search for a model perpendicular to.
        * \param[in] ax the axis along which we need to search for a model perpendicular to
        */
      inline void 
      setAxis (const Eigen::Vector3f &ax) { axis_ = ax; }

      /** \brief Get the axis along which we need to search for a model perpendicular to. */
      inline Eigen::Vector3f 
      getAxis () const { return (axis_); }

      /** \brief Set the angle epsilon (delta) threshold.
        * \param[in] ea the maximum allowed difference between the model normal and the given axis in radians.
        */
      inline void 
      setEpsAngle (double ea) { eps_angle_ = ea; }

      /** \brief Get the epsilon (delta) model angle threshold in radians. */
      inline double 
      getEpsAngle () const { return (eps_angle_); }

      /** \brief Base method for segmentation of a model in a PointCloud given by <setInputCloud (), setIndices ()>
        * \param[in] inliers the resultant point indices that support the model found (inliers)
        * \param[out] model_coefficients the resultant model coefficients
        */
      virtual void 
      segment (PointIndices &inliers, ModelCoefficients &model_coefficients);

    protected:
      /** \brief Initialize the Sample Consensus model and set its parameters.
        * \param[in] model_type the type of SAC model that is to be used
        */
      virtual bool 
      initSACModel (const int model_type);

      /** \brief Initialize the Sample Consensus method and set its parameters.
        * \param[in] method_type the type of SAC method to be used
        */
      virtual void 
      initSAC (const int method_type);

      /** \brief The model that needs to be segmented. */
      SampleConsensusModelPtr model_;

      /** \brief The sample consensus segmentation method. */
      SampleConsensusPtr sac_;

      /** \brief The type of model to use (user given parameter). */
      int model_type_;

      /** \brief The type of sample consensus method to use (user given parameter). */
      int method_type_;

      /** \brief Distance to the model threshold (user given parameter). */
      double threshold_;

      /** \brief Set to true if a coefficient refinement is required. */
      bool optimize_coefficients_;

      /** \brief The minimum and maximum radius limits for the model. Applicable to all models that estimate a radius. */
      double radius_min_, radius_max_;

      /** \brief The maximum allowed difference between the model normal and the given axis. */
      double eps_angle_;

      /** \brief The axis along which we need to search for a model perpendicular to. */
      Eigen::Vector3f axis_;

      /** \brief Maximum number of iterations before giving up (user given parameter). */
      int max_iterations_;

      /** \brief Desired probability of choosing at least one sample free from outliers (user given parameter). */
      double probability_;

      /** \brief Class get name method. */
      virtual std::string 
      getClassName () const { return ("SACSegmentation"); }
  };

  /** \brief @b SACSegmentationFromNormals represents the PCL nodelet segmentation class for Sample Consensus methods and
    * models that require the use of surface normals for estimation.
    * \ingroup segmentation
    */
  template <typename PointT, typename PointNT>
  class SACSegmentationFromNormals: public SACSegmentation<PointT>
  {
    using SACSegmentation<PointT>::model_;
    using SACSegmentation<PointT>::model_type_;
    using SACSegmentation<PointT>::radius_min_;
    using SACSegmentation<PointT>::radius_max_;
    using SACSegmentation<PointT>::eps_angle_;
    using SACSegmentation<PointT>::axis_;

    public:
      using PCLBase<PointT>::input_;
      using PCLBase<PointT>::indices_;

      typedef typename SACSegmentation<PointT>::PointCloud PointCloud;
      typedef typename PointCloud::Ptr PointCloudPtr;
      typedef typename PointCloud::ConstPtr PointCloudConstPtr;

      typedef typename pcl::PointCloud<PointNT> PointCloudN;
      typedef typename PointCloudN::Ptr PointCloudNPtr;
      typedef typename PointCloudN::ConstPtr PointCloudNConstPtr;

      typedef typename SampleConsensus<PointT>::Ptr SampleConsensusPtr;
      typedef typename SampleConsensusModel<PointT>::Ptr SampleConsensusModelPtr;
      typedef typename SampleConsensusModelFromNormals<PointT, PointNT>::Ptr SampleConsensusModelFromNormalsPtr;

      /** \brief Empty constructor. */
      SACSegmentationFromNormals () : distance_weight_ (0.1) {};

      /** \brief Provide a pointer to the input dataset that contains the point normals of 
        * the XYZ dataset.
        * \param[in] normals the const boost shared pointer to a PointCloud message
        */
      inline void 
      setInputNormals (const PointCloudNConstPtr &normals) { normals_ = normals; }

      /** \brief Get a pointer to the normals of the input XYZ point cloud dataset. */
      inline PointCloudNConstPtr 
      getInputNormals () const { return (normals_); }

      /** \brief Set the relative weight (between 0 and 1) to give to the angular 
        * distance (0 to pi/2) between point normals and the plane normal.
        * \param[in] distance_weight the distance/angular weight
        */
      inline void 
      setNormalDistanceWeight (double distance_weight) { distance_weight_ = distance_weight; }

      /** \brief Get the relative weight (between 0 and 1) to give to the angular distance (0 to pi/2) between point
        * normals and the plane normal. */
      inline double 
      getNormalDistanceWeight () const { return (distance_weight_); }

      /** \brief Set the distance we expect a plane model to be from the origin
        * \param[in] d distance from the template plane modl to the origin
        */
      inline void
      setDistanceFromOrigin (const double d) { distance_from_origin_ = d; }

      /** \brief Get the distance of a plane model from the origin. */
      inline double
      getDistanceFromOrigin () const { return (distance_from_origin_); }

    protected:
      /** \brief A pointer to the input dataset that contains the point normals of the XYZ dataset. */
      PointCloudNConstPtr normals_;

      /** \brief The relative weight (between 0 and 1) to give to the angular
        * distance (0 to pi/2) between point normals and the plane normal. 
        */
      double distance_weight_;

      /** \brief The distance from the template plane to the origin. */
      double distance_from_origin_;

      /** \brief Initialize the Sample Consensus model and set its parameters.
        * \param[in] model_type the type of SAC model that is to be used
        */
      virtual bool 
      initSACModel (const int model_type);

      /** \brief Class get name method. */
      virtual std::string 
      getClassName () const { return ("SACSegmentationFromNormals"); }
  };
}

#endif  //#ifndef PCL_SEGMENTATION_SAC_SEGMENTATION_H_
