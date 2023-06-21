#include <gst_pipes_plugin_webrtc_base.h>

namespace gst_pipes
{
void gst_pipes_webrtc_base::initialise(
  std::string name,  // the config name of the plugin
  std::shared_ptr<gst_bridge::node_interface_collection> node_if, GstElement * pipeline)
{
  name_ = name;
  node_if_ = node_if;
  pipeline_ = pipeline;

  elem_name_ = node_if->parameters
                 ->declare_parameter(
                   name + ".element_name", rclcpp::ParameterValue("mysrc"),
                   descr("the name of the source element inside the pipeline", true))
                 .get<std::string>();

  if (GST_IS_BIN(pipeline_)) {
    GstElement * bin = gst_bin_get_by_name(GST_BIN_CAST(pipeline_), elem_name_.c_str());
    if (bin) {
      RCLCPP_INFO(
        node_if->logging->get_logger(), "plugin gst_pipes_webrtc '%s' found '%s'",
        name_.c_str(), elem_name_.c_str());

      // XXX test if bin_ is a webrtcbin
      webrtc_ = bin;


      // webrtc peer discovery server comms:
      // webrtc_on_negotiation_needed is called when webrtc goes to playing
      g_signal_connect(webrtc_, "on-negotiation-needed", G_CALLBACK(on_negotiation_needed_cb), this);

      // gstreamer will compute and emit ICE candidates, these need to be sent to the remote server
      g_signal_connect(webrtc_, "on-ice-candidate", G_CALLBACK(on_ice_candidate_cb), this);

      // the remote server will send messages back, these need to be forwarded to webrtcbin


      // this is a monitoring call for quality of life, it tells us where in the ICE candidate step we're up to
      g_signal_connect(webrtc_, "notify::ice-gathering-state", G_CALLBACK(on_notify_ice_gathering_state_cb), this);


      // connecting calls:
      // webrtc will add new pads for audio or video, these need decoding, it's easy to just plug a decodebin into here
      g_signal_connect(webrtc_, "pad-added", G_CALLBACK(pad_added_cb), this);

      /* data channels requires gstreamer 1.18 or higher
      g_signal_connect(webrtc_, "on-data-channel", G_CALLBACK(on_data_channel_cb), this);

      
      // data channels can be used to create tunnels for ROS serialised transports

      // the remote peer is offering a low latency data channel 
      // we can offer a low latency data channel back
      g_signal_emit_by_name(webrtc_, "create-data-channel", "channel", NULL, &data_channel_tx);

      */
      init_signalling_server_client();

    }

    else {
      RCLCPP_ERROR(
        node_if_->logging->get_logger(),
        "plugin gst_pipes_webrtc '%s' failed to locate a gstreamer element called '%s'",
        name_.c_str(), elem_name_.c_str());
    }
  } else {
    RCLCPP_ERROR(
      node_if_->logging->get_logger(),
      "plugin gst_pipes_webrtc '%s' received invalid pipeline in initialisation",
      name_.c_str());
  }
}





  // ############ virtual methods for different signalling servers ############

  // connect to your signalling server
  void gst_pipes_webrtc_base::init_signalling_server_client(){}

  // called when the webrtcbin wants to send a SDP answer
  // default calls  send_sdp(descr)
  void gst_pipes_webrtc_base::send_sdp_answer(
    GstWebRTCSessionDescription * desc
  ){
    RCLCPP_INFO(
      node_if_->logging->get_logger(),
      "plugin gst_pipes_webrtc '%s' sending sdp answer",
      name_.c_str());

    send_sdp(desc);
  }

  // called when the webrtcbin is instructed to send a sdp offer
  // default calls  send_sdp(descr)
  void gst_pipes_webrtc_base::send_sdp_offer(
    GstWebRTCSessionDescription * desc
  ){
    RCLCPP_INFO(
      node_if_->logging->get_logger(),
      "plugin gst_pipes_webrtc '%s' sending sdp offer",
      name_.c_str());

    send_sdp(desc);
  }

  // send a sdp description to the remote server
  void gst_pipes_webrtc_base::send_sdp(
    GstWebRTCSessionDescription * desc
  ){
    (void) desc;
  }

  // send an ice candidat to the remote server
  void gst_pipes_webrtc_base::send_ice_candidate(
    guint mline_index,
    gchararray candidate
  ){
    (void) mline_index;
    (void) candidate;
  }





// ############### C style callbacks for the webrtc async background logic ###############

void
gst_pipes_webrtc_base::on_negotiation_needed_cb(
  GstElement * object,
  gpointer user_data
){
  (void) object;
  gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
  this_ptr->begin_negotiate();
}


void
gst_pipes_webrtc_base::on_ice_candidate_cb(
  GstElement * object,
  guint mline_index,
  gchararray candidate,
  gpointer user_data
){
  (void) object;
  gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
  // our webrtcbin is ready to send an ice candidate to the remote peer,
  // XXX package and publish the candidate
  this_ptr->send_ice_candidate(mline_index, candidate);
}


void
gst_pipes_webrtc_base::on_notify_ice_gathering_state_cb(
  GstElement *webrtcbin,
  GParamSpec *pspec,
  gpointer user_data
){
  (void) webrtcbin;
  (void) pspec;
  (void) user_data;
  //gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
  
  // XXX this is purely a monitoring call, just print to debug, and maybe send a ros diagnostics update
}


void
gst_pipes_webrtc_base::pad_added_cb(
  GstElement *webrtc,
  GstPad *pad,
  gpointer user_data
){
  (void) webrtc;
  (void) pad;
  (void) user_data;
  //gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
  
  // XXX test if the pad is audio or video, then build and attach the appropriate sink bin
  // optionally shim a decodebin in for convenience
}


// ############### async logic - local sends a SDP offer ###############

// signal the webrtcbin to create an offer,
// take the offer from the the webrtcbin and send it to the remote peer via the signalling layer
// accept a sdp answer back 



// entry point to establishing a call
void
gst_pipes_webrtc_base::create_offer()
{
    GstPromise *promise =
      gst_promise_new_with_change_func (create_offer_prom, this, NULL);
    g_signal_emit_by_name (webrtc_, "create-offer", NULL, promise);
}


// on_offer_created
// unpack the promise and retrieve the sdp offer, calling sdp_received with the result
void
gst_pipes_webrtc_base::create_offer_prom (GstPromise * promise, gpointer user_data)
{
  gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;

  GstWebRTCSessionDescription *offer = NULL;
  const GstStructure *reply;

  // g_assert_cmphex (this_ptr->app_state, ==, PEER_CALL_NEGOTIATING); // XXX websockets related

  // retrieve the offer from the promise
  g_assert_cmphex (gst_promise_wait (promise), ==, GST_PROMISE_RESULT_REPLIED);
  reply = gst_promise_get_reply (promise);
  gst_structure_get (reply, "offer",
      GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
  gst_promise_unref (promise);

  // set the local description
  // XXX this is probably not needed, or needs to be broken out for overloading
  promise = gst_promise_new ();
  g_signal_emit_by_name (this_ptr->webrtc_, "set-local-description", offer, promise);
  gst_promise_interrupt (promise);
  gst_promise_unref (promise);

  // Send offer to peer
  this_ptr->send_sdp_offer (offer);
  gst_webrtc_session_description_free (offer);

}

// after sending the offer, we expect the client link to return a sdp answer
// convenience overload to unpack the GstSDPMessage to a GstWebRTCSessionDescription
void
gst_pipes_webrtc_base::sdp_answer_received (GstSDPMessage * sdp)
{
  GstWebRTCSessionDescription *answer = NULL;

  answer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER, sdp);
  sdp_answer_received(answer);
}

void
gst_pipes_webrtc_base::sdp_answer_received (GstWebRTCSessionDescription * answer)
{
  g_assert_nonnull (answer);
  
  GstPromise *promise = gst_promise_new ();
  g_signal_emit_by_name (webrtc_, "set-remote-description", answer,
    promise);
  gst_promise_interrupt (promise);
  gst_promise_unref (promise);
  // XXX mark the call as complete


}

// ############### async logic - remote sent a SDP offer ###############


  // sdp_offer_received sends signal "set-remote-description"
  // whose promise calls set_remote_description_prom
  // which emits signal g_signal_emit_by_name(webrtc_, "create-answer", NULL, promise);
  // whose promise calls create_answer_prom
  // which emits signal g_signal_emit_by_name(webrtc_, "set-local-description", answer, promise);
  // whose promise can finally terminate the call tree

// called when the remote peer sends a SDP offer,
// this will be called by a ros topic message in the default plugin
// convenience overload to unpack the GstSDPMessage to a GstWebRTCSessionDescription

void
gst_pipes_webrtc_base::sdp_offer_received (GstSDPMessage * sdp)
{
  GstWebRTCSessionDescription *offer = NULL;

  offer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_OFFER, sdp);

  /* Set remote description on our pipeline */
  sdp_offer_received(offer);
  gst_webrtc_session_description_free (offer);
}



// called when the remote peer sends a SDP offer,
// this will be called by a ros topic message in the default plugin
void
gst_pipes_webrtc_base::sdp_offer_received(
  GstWebRTCSessionDescription * offer
){
  g_assert_nonnull (offer);

  GstPromise * promise = gst_promise_new_with_change_func(set_remote_description_prom, this, NULL);

  g_signal_emit_by_name(webrtc_, "set-remote-description", offer, promise);

}

void 
gst_pipes_webrtc_base::set_remote_description_prom(
  GstPromise *promise,
  gpointer user_data
){
  gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
  gst_promise_unref(promise);


  promise = gst_promise_new_with_change_func(create_answer_prom, (gpointer) this_ptr, NULL);
  g_signal_emit_by_name(this_ptr->webrtc_, "create-answer", NULL, promise);

}


void 
gst_pipes_webrtc_base::create_answer_prom(
  GstPromise *promise,
  gpointer user_data
){
  gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;

  GstWebRTCSessionDescription *answer = NULL;
  const GstStructure *reply;

  // XXX sanity check the state of the promise

  // extract answer from finished promise
  reply = gst_promise_get_reply(promise);
  gst_structure_get(reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
  gst_promise_unref(promise);

  // Is this needed?
  g_signal_emit_by_name(this_ptr->webrtc_, "set-local-description", answer, NULL);

  // XXX send answer to remote peer,
  this_ptr->send_sdp_answer(answer);

}

// ############### async background: ice candidates ###############


// called when the remote peer sends an ice candidate,
// this will be called by a ros topic message in the default plugin
void
gst_pipes_webrtc_base::ice_candidate_received(
  guint mline_index,          // the index of the media description in the SDP
  const gchar* ice_candidate    // an ice candidate or NULL/"" to mark that no more candidates will arrive
){
  g_signal_emit_by_name(webrtc_, "add-ice-candidate", mline_index, ice_candidate);
}


// ############### data channel callbacks ###############


// remote peer has offered a data channel to send us data
void
gst_pipes_webrtc_base::on_data_channel_cb(
  GstElement * object,
  GstWebRTCDataChannel * channel,
  gpointer user_data
){
  (void) object;
  gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
  this_ptr->data_channel_rx_ = channel; // XXX use a setter method

  // events associated with data channels:
  g_signal_connect(channel, "on-error", G_CALLBACK(data_channel_on_error_cb), this_ptr);
  g_signal_connect(channel, "on-open", G_CALLBACK(data_channel_on_open_cb), this_ptr);
  g_signal_connect(channel, "on-close", G_CALLBACK(data_channel_on_close_cb), this_ptr);
  g_signal_connect(channel, "on-message-data ", G_CALLBACK(data_channel_on_message_data_cb), this_ptr);
}

void
gst_pipes_webrtc_base::data_channel_on_message_data_cb(
  GstWebRTCDataChannel * self,
  GBytes * data,
  gpointer user_data
){
  (void) self;
  (void) data;
  (void) user_data;
  // gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
}

void
gst_pipes_webrtc_base::data_channel_on_open_cb(
  GstWebRTCDataChannel * self,
  gpointer user_data
){
  (void) self;
  (void) user_data;
  // gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
}

void
gst_pipes_webrtc_base::data_channel_on_error_cb(
  GstWebRTCDataChannel * self,
  GError * error,
  gpointer user_data
){
  (void) self;
  (void) error;
  (void) user_data;
  // gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
}

void
gst_pipes_webrtc_base::data_channel_on_close_cb(
  GstWebRTCDataChannel * self,
  gpointer user_data
){
  (void) self;
  (void) user_data;
  // gst_pipes_webrtc_base* this_ptr = (gst_pipes_webrtc_base*) user_data;
}



}  // namespace gst_pipes

