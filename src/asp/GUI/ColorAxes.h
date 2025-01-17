// __BEGIN_LICENSE__
//  Copyright (c) 2006-2013, United States Government as represented by the
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


/// \file ColorAxes.h
///
/// A widget showing colorized data with a colormap and axes
///

#ifndef __STEREO_GUI_COLOR_AXES_H__
#define __STEREO_GUI_COLOR_AXES_H__

#include <QWidget>
#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>

namespace vw { namespace gui {

class imageData;
class ColorAxesPlotter;
  
class ColorAxes: public QwtPlot {
  Q_OBJECT
  
public:
  ColorAxes(QWidget * parent, imageData & image);

  virtual void mousePressEvent(QMouseEvent *e);

  virtual void resizeEvent(QResizeEvent *e);
  
public Q_SLOTS:

private:
  ColorAxesPlotter *m_plotter;
  imageData & m_image;
};

}}
  
#endif  // __STEREO_GUI_COLOR_AXES_H__
