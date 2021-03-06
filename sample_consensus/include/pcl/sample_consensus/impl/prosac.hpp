/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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
 * $Id: prosac.hpp 1370 2011-06-19 01:06:01Z jspricke $
 *
 */

#ifndef PCL_SAMPLE_CONSENSUS_IMPL_PROSAC_H_
#define PCL_SAMPLE_CONSENSUS_IMPL_PROSAC_H_

#include <boost/math/distributions/binomial.hpp>
#include "pcl/sample_consensus/prosac.h"

//////////////////////////////////////////////////////////////////////////
// Variable naming uses capital letters to make the comparison with the original paper easier
template<typename PointT> bool 
pcl::ProgressiveSampleConsensus<PointT>::computeModel (int debug_verbosity_level)
{
  // Warn and exit if no threshold was set
  if (threshold_ == DBL_MAX)
  {
    PCL_ERROR ("[pcl::ProgressiveSampleConsensus::computeModel] No threshold set!\n");
    return (false);
  }

  // Initialize some PROSAC constants
  float T_N = 200000;
  unsigned int N = sac_model_->indices_->size ();
  unsigned int m = sac_model_->getSampleSize ();
  float T_n = T_N;
  for (unsigned int i = 0; i < m; ++i)
    T_n *= (float)(m - i) / (float)(N - i);
  float T_prime_n = 1;
  unsigned int I_N_best = 0;
  float n = m;

  // Define the n_Start coefficients from Section 2.2
  float n_star = N;
  unsigned int I_n_star = 0;
  float epsilon_n_star = (float)I_n_star / n_star;
  unsigned int k_n_star = T_N;

  // Compute the I_n_star_min of Equation 8
  std::vector<unsigned int> I_n_star_min (N);

  // Initialize the usual RANSAC parameters
  iterations_ = 0;

  std::vector<int> inliers;
  std::vector<int> selection;
  Eigen::VectorXf model_coefficients;

  // We will increase the pool so the indices_ vector can only contain m elements at first
  std::vector<int> index_pool;
  index_pool.reserve (N);
  for (unsigned int i = 0; i < n; ++i)
    index_pool.push_back (sac_model_->indices_->operator[](i));

  // Iterate
  while ((unsigned int)iterations_ < k_n_star)
  {
    // Choose the samples

    // Step 1
    // According to Equation 5 in the text text, not the algorithm
    if ((iterations_ == T_prime_n) && (n < n_star))
    {
      // Increase the pool
      ++n;
      if (n >= N)
        break;
      index_pool.push_back (sac_model_->indices_->at(n - 1));
      // Update other variables
      float T_n_minus_1 = T_n;
      T_n *= (float)(n + 1) / (float)(n + 1 - m);
      T_prime_n += ceil(T_n - T_n_minus_1);
    }

    // Step 2
    sac_model_->indices_->swap (index_pool);
    selection.clear ();
    sac_model_->getSamples (iterations_, selection);
    if (T_prime_n < iterations_)
    {
      selection.pop_back ();
      selection.push_back (sac_model_->indices_->at(n - 1));
    }

    // Make sure we use the right indices for testing
    sac_model_->indices_->swap (index_pool);

    if (selection.empty ())
    {
      PCL_ERROR ("[pcl::ProgressiveSampleConsensus::computeModel] No samples could be selected!\n");
      break;
    }

    // Search for inliers in the point cloud for the current model
    if (!sac_model_->computeModelCoefficients (selection, model_coefficients))
    {
      ++iterations_;
      continue;
    }

    // Select the inliers that are within threshold_ from the model
    inliers.clear ();
    sac_model_->selectWithinDistance (model_coefficients, threshold_, inliers);

    unsigned int I_N = inliers.size ();

    // If we find more inliers than before
    if (I_N > I_N_best)
    {
      I_N_best = I_N;

      // Save the current model/inlier/coefficients selection as being the best so far
      inliers_ = inliers;
      model_ = selection;
      model_coefficients_ = model_coefficients;

      // We estimate I_n_star for different possible values of n_star by using the inliers
      std::sort (inliers.begin (), inliers.end ());

      // Try to find a better n_star
      // We minimize k_n_star and therefore maximize epsilon_n_star = I_n_star / n_star
      unsigned int possible_n_star_best = N, I_possible_n_star_best = I_N;
      float epsilon_possible_n_star_best = (float)I_possible_n_star_best / possible_n_star_best;

      // We only need to compute possible better epsilon_n_star for when _n is just about to be removed an inlier
      unsigned int I_possible_n_star = I_N;
      for (std::vector<int>::const_reverse_iterator last_inlier = inliers.rbegin (), 
                                                    inliers_end = inliers.rend (); 
           last_inlier != inliers_end; 
           ++last_inlier, --I_possible_n_star)
      {
        // The best possible_n_star for a given I_possible_n_star is the index of the last inlier
        unsigned int possible_n_star = (*last_inlier) + 1;
        if (possible_n_star <= m)
          break;

        // If we find a better epsilon_n_star
        float epsilon_possible_n_star = (float)I_possible_n_star / possible_n_star;
        // Make sure we have a better epsilon_possible_n_star
        if ((epsilon_possible_n_star > epsilon_n_star) && (epsilon_possible_n_star > epsilon_possible_n_star_best))
        {
          using namespace boost::math;
          // Typo in Equation 7, not (n-m choose i-m) but (n choose i-m)
          unsigned int
                       I_possible_n_star_min = m
                           + ceil (quantile (complement (binomial_distribution<float>(possible_n_star, 0.1), 0.05)));
          // If Equation 9 is not verified, exit
          if (I_possible_n_star < I_possible_n_star_min)
            break;

          possible_n_star_best = possible_n_star;
          I_possible_n_star_best = I_possible_n_star;
          epsilon_possible_n_star_best = epsilon_possible_n_star;
        }
      }

      // Check if we get a better epsilon
      if (epsilon_possible_n_star_best > epsilon_n_star)
      {
        // update the best value
        epsilon_n_star = epsilon_possible_n_star_best;

        // Compute the new k_n_star
        float bottom_log = 1 - std::pow (epsilon_n_star, static_cast<float>(m));
        if (bottom_log == 0)
          k_n_star = 1;
        else if (bottom_log == 1)
          k_n_star = T_N;
        else
          k_n_star = (int)ceil (log(0.05) / log (bottom_log));
        // It seems weird to have very few iterations, so do have a few (totally empirical)
        k_n_star = (std::max)(k_n_star, 2 * m);
      }
    }

    ++iterations_;
    if (debug_verbosity_level > 1)
      PCL_DEBUG ("[pcl::ProgressiveSampleConsensus::computeModel] Trial %d out of %d: %d inliers (best is: %d so far).\n", iterations_, k_n_star, I_N, I_N_best);
    if (iterations_ > max_iterations_)
    {
      if (debug_verbosity_level > 0)
        PCL_DEBUG ("[pcl::ProgressiveSampleConsensus::computeModel] RANSAC reached the maximum number of trials.\n");
      break;
    }
  }

  if (debug_verbosity_level > 0)
    PCL_DEBUG ("[pcl::ProgressiveSampleConsensus::computeModel] Model: %lu size, %d inliers.\n", (unsigned long)model_.size (), I_N_best);

  if (model_.empty ())
  {
    inliers_.clear ();
    return (false);
  }

  // Get the set of inliers that correspond to the best model found so far
  //sac_model_->selectWithinDistance (model_coefficients_, threshold_, inliers_);
  return (true);
}

#define PCL_INSTANTIATE_ProgressiveSampleConsensus(T) template class PCL_EXPORTS pcl::ProgressiveSampleConsensus<T>;

#endif    // PCL_SAMPLE_CONSENSUS_IMPL_PROSAC_H_
