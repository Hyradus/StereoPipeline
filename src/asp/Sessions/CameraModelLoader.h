// __BEGIN_LICENSE__
//  Copyright (c) 2009-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NGT platform is licensed under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance with the
//  License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


/// \file CameraModelLoader.h
///

#ifndef __STEREO_SESSION_CAMERAMODELLOADER_H__
#define __STEREO_SESSION_CAMERAMODELLOADER_H__


#include <vw/Camera/CameraModel.h>

namespace asp {

  class CameraModelLoader {
  public:

    // Setup/teardown code is handled here
    // - Currently this just means the Xerces XML init/deinit functions.
    CameraModelLoader();
    ~CameraModelLoader();

    // TODO: Add a generic loading function.

    // Camera model loading functions
    vw::CamPtr load_rpc_camera_model        (std::string const& path) const;
    vw::CamPtr load_dg_camera_model         (std::string const& path) const;
    vw::CamPtr load_pinhole_camera_model    (std::string const& path) const;
    vw::CamPtr load_isis_camera_model       (std::string const& path) const;
    vw::CamPtr load_spot5_camera_model      (std::string const& path) const;
    vw::CamPtr load_perusat_camera_model    (std::string const& path) const;
    vw::CamPtr load_pleiades_camera_model   (std::string const& path) const;
    vw::CamPtr load_ASTER_camera_model      (std::string const& path) const;
    vw::CamPtr load_optical_bar_camera_model(std::string const& path) const;
    vw::CamPtr load_csm_camera_model        (std::string const& path) const;
  }; // End class CameraModelLoader

  
  // Find the underlying CSM camera. Applies only to CSM, Pleiades, and DG.
  class CsmModel;
  CsmModel * csm_model(boost::shared_ptr<vw::camera::CameraModel> cam,
                       std::string const& stereo_session);
  
} // end namespace asp

#endif // __STEREO_SESSION_CAMERAMODELLOADER_H__
