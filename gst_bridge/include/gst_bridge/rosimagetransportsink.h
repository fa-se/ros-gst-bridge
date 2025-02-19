/* gst_bridge
 * Copyright (C) 2020-2021 Brett Downing <brettrd@brettrd.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _GST_ROSIMAGETRANSPORTSINK_H_
#define _GST_ROSIMAGETRANSPORTSINK_H_

#include <gst/base/gstbasesink.h>
#include <gst/video/video-format.h>
#include <gst_bridge/gst_bridge.h>
#include <gst_bridge/rosbasesink.h>

//include ROS and ROS message formats
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <image_transport/image_transport.hpp>

G_BEGIN_DECLS

#define GST_TYPE_ROSIMAGETRANSPORTSINK (rosimagetransportsink_get_type())
#define GST_ROSIMAGETRANSPORTSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_ROSIMAGETRANSPORTSINK, Rosimagetransportsink))
#define GST_ROSIMAGETRANSPORTSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_ROSIMAGETRANSPORTSINK, RosimagetransportsinkClass))
#define GST_IS_ROSIMAGETRANSPORTSINK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_ROSIMAGETRANSPORTSINK))
#define GST_IS_ROSIMAGETRANSPORTSINK_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_ROSIMAGETRANSPORTSINK))

typedef struct _Rosimagetransportsink Rosimagetransportsink;
typedef struct _RosimagetransportsinkClass RosimagetransportsinkClass;

struct _Rosimagetransportsink
{
  RosBaseSink parent;

  gchar * pub_topic;
  gchar * frame_id;
  gchar * encoding;   //image topic encoding string
  gchar * ffmpeg_transport_encoder; // encoder to be used by ffmpeg-image-transport (if available)
  gchar * init_caps;  //optional caps override (used for limited apis)

  image_transport::Publisher pub;

  int height;
  int width;

  size_t step;  //bytes per pixel
  gint endianness;
};

struct _RosimagetransportsinkClass
{
  RosBaseSinkClass parent_class;

  // stick member function pointers here
  // along with member function pointers for signal handlers
};

GType rosimagetransportsink_get_type(void);

G_END_DECLS

#endif
