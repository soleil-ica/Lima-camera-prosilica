#include <signal.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lima/Exceptions.h"

#include "ProsilicaCamera.h"
#include "ProsilicaSyncCtrlObj.h"
#include "ProsilicaVideoCtrlObj.h"

using namespace lima;
using namespace lima::Prosilica;


Camera::Camera(const char *ip_addr,bool master,
                bool mono_forced) :
  m_cam_connected(false),
  m_sync(NULL),
  m_video(NULL),
  m_bin(1,1),
  m_mono_forced(mono_forced)
{
  DEB_CONSTRUCTOR();
  //Tango signal management is a real shit (workaround)
  sigset_t signals;
  sigfillset(&signals);
  sigprocmask(SIG_UNBLOCK,&signals,NULL);

  // Init Frames
  m_frame[0].ImageBuffer = NULL;
  m_frame[0].Context[0] = this;
  m_frame[1].ImageBuffer = NULL;
  m_frame[1].Context[0] = this;
  
  m_camera_name[0] = m_sensor_type[0] = '\0';
  unsigned long ip = inet_addr(ip_addr);
  tPvErr error = PvInitialize();
  if(error)
    throw LIMA_HW_EXC(Error, "could not initialize Prosilica API");

  m_cam_connected = !PvCameraOpenByAddr(ip,
					master ? ePvAccessMaster : ePvAccessMonitor,
					&m_handle);
  if(!m_cam_connected)
    throw LIMA_HW_EXC(Error, "Camera not found!");

  unsigned long psize;
  PvAttrStringGet(m_handle, "CameraName", m_camera_name, 128, &psize);
  PvAttrUint32Get(m_handle, "UniqueId", &m_uid);
  PvAttrUint32Get(m_handle, "FirmwareVerMajor", &m_ufirmware_maj);
  PvAttrUint32Get(m_handle, "FirmwareVerMinor", &m_ufirmware_min);
  PvAttrEnumGet(m_handle, "SensorType", m_sensor_type, 
		sizeof(m_sensor_type), &psize);

  DEB_TRACE() << DEB_VAR3(m_camera_name,m_sensor_type,m_uid);

  PvAttrUint32Get(m_handle, "SensorWidth", &m_maxwidth);
  PvAttrUint32Get(m_handle, "SensorHeight", &m_maxheight);

  DEB_TRACE() << DEB_VAR2(m_maxwidth,m_maxheight);


  Bin tmp_bin(1, 1);
  if(master)
    {
      setBin(tmp_bin); // Bin has to be (1,1) for allowing maximum values as width and height

      error = PvAttrUint32Set(m_handle,"Width",m_maxwidth);
      if(error)
	throw LIMA_HW_EXC(Error,"Can't set image width");

      error = PvAttrUint32Set(m_handle,"Height",m_maxheight);
      if(error)
	throw LIMA_HW_EXC(Error,"Can't set image height");


      VideoMode localVideoMode;
      if(isMonochrome())
	{
	  error = PvAttrEnumSet(m_handle, "PixelFormat", "Mono16");
	  localVideoMode = Y16;
	  if (error && m_mono_forced)
	    {
	      error = PvAttrEnumSet(m_handle, "PixelFormat", "Mono8");
	      localVideoMode = Y8;
	    }
	}
      else
	{
	  error = PvAttrEnumSet(m_handle, "PixelFormat", "Bayer16");
	  localVideoMode = BAYER_RG16;
	}

      if(error)
	throw LIMA_HW_EXC(Error,"Can't set image format");
  
      m_video_mode = localVideoMode;

      error = PvAttrEnumSet(m_handle, "AcquisitionMode", "Continuous");
      if(error)
	throw LIMA_HW_EXC(Error,"Can't set acquisition mode to continuous");
    }
  else
    m_video_mode = Y8;
  
  m_as_master = master;
}

Camera::~Camera()
{
  DEB_DESTRUCTOR();

  if(m_cam_connected)
    {
      PvCommandRun(m_handle,"AcquisitionStop");
      PvCaptureEnd(m_handle);
      PvCameraClose(m_handle);
    }
  PvUnInitialize();
  if(m_frame[0].ImageBuffer)
    free(m_frame[0].ImageBuffer);
  if(m_frame[1].ImageBuffer)
    free(m_frame[1].ImageBuffer);
}

/** @brief test if the camera is monochrome
 */
bool Camera::isMonochrome() const
{
  DEB_MEMBER_FUNCT();

  return (!strcmp(m_sensor_type,"Mono") || m_mono_forced);
}

VideoMode Camera::getVideoMode() const
{
  DEB_MEMBER_FUNCT();
  DEB_RETURN() << DEB_VAR1(m_video_mode);

  return m_video_mode;
}

void Camera::getCameraName(std::string& name)
{
  DEB_MEMBER_FUNCT();
  DEB_RETURN() << DEB_VAR1(m_camera_name);

  name = m_camera_name;
}
void Camera::setVideoMode(VideoMode aMode)
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(aMode);

  ImageType anImageType;
  tPvErr error;
  switch(aMode)
    {
    case Y8:
      error = PvAttrEnumSet(m_handle, "PixelFormat", "Mono8");
      anImageType = Bpp8;
      break;
    case Y16:
      error = PvAttrEnumSet(m_handle, "PixelFormat", "Mono16");
      anImageType = Bpp16;
      break;
    case BAYER_RG8:
      error = PvAttrEnumSet(m_handle, "PixelFormat", "Bayer8");
      anImageType = Bpp8;
      break;
    case BAYER_RG16:
      error = PvAttrEnumSet(m_handle, "PixelFormat", "Bayer16");
      anImageType = Bpp16;
      break;
    case RGB24:
      error = PvAttrEnumSet(m_handle, "PixelFormat", "Rgb24");
      anImageType = Bpp8;
      break;
    case BGR24:
      error = PvAttrEnumSet(m_handle, "PixelFormat", "Bgr24");
      anImageType = Bpp8;
      break;
    default:
      throw LIMA_HW_EXC(InvalidValue,"This video mode is not managed!");
    }
  
  if(error)
    throw LIMA_HW_EXC(Error,"Can't change video mode");
  
  m_video_mode = aMode;
  maxImageSizeChanged(Size(m_maxwidth,m_maxheight),anImageType);
}

void Camera::_allocBuffer()
{
  DEB_MEMBER_FUNCT();

  tPvUint32 imageSize;
  tPvErr error = PvAttrUint32Get(m_handle, "TotalBytesPerFrame", &imageSize);
  if(error)
    throw LIMA_HW_EXC(Error,"Can't get camera image size");

  DEB_TRACE() << DEB_VAR1(imageSize);
  //realloc
  if(!m_frame[0].ImageBuffer || m_frame[0].ImageBufferSize < imageSize)
    {
      //Frame 0
      m_frame[0].ImageBuffer = realloc(m_frame[0].ImageBuffer,
				       imageSize);
      m_frame[0].ImageBufferSize = imageSize;

      //Frame 1
      m_frame[1].ImageBuffer = realloc(m_frame[1].ImageBuffer,
				       imageSize);

      m_frame[1].ImageBufferSize = imageSize;
    }
}
/** @brief start the acquisition.
    must have m_video != NULL and previously call _allocBuffer
*/
void Camera::startAcq()
{
  DEB_MEMBER_FUNCT();

  m_continue_acq = true;
  m_acq_frame_nb = 0;
  tPvErr error = PvCaptureQueueFrame(m_handle,&m_frame[0],_newFrameCBK);

  int requested_nb_frames;
  m_sync->getNbFrames(requested_nb_frames);
  bool isLive;
  m_video->getLive(isLive);

  if(!requested_nb_frames || requested_nb_frames > 1 || isLive)
    error = PvCaptureQueueFrame(m_handle,&m_frame[1],_newFrameCBK);
}

void Camera::reset()
{
  DEB_MEMBER_FUNCT();
  //@todo maybe something to do!
}

void Camera::_newFrameCBK(tPvFrame* aFrame)
{
  DEB_STATIC_FUNCT();
  Camera *aCamera = (Camera*)aFrame->Context[0];
  aCamera->_newFrame(aFrame);
}

void Camera::_newFrame(tPvFrame* aFrame)
{
  DEB_MEMBER_FUNCT();

  if(!m_continue_acq) return;

  if(aFrame->Status != ePvErrSuccess)
    {
      if(aFrame->Status != ePvErrCancelled)
	{
	  DEB_WARNING() << DEB_VAR1(aFrame->Status);
	  PvCaptureQueueFrame(m_handle,aFrame,_newFrameCBK);
	}
      return;
    }
  
  int requested_nb_frames;
  m_sync->getNbFrames(requested_nb_frames);
  bool isLive;
  m_video->getLive(isLive);
  ++m_acq_frame_nb;

  bool stopAcq = false;
  if(isLive || !requested_nb_frames || m_acq_frame_nb < (requested_nb_frames - 1))
    {
      if(isLive || !requested_nb_frames ||
	 m_acq_frame_nb < (requested_nb_frames - 2))
	tPvErr error = PvCaptureQueueFrame(m_handle,aFrame,_newFrameCBK);
    }
  else
    stopAcq = true;
  
  VideoMode mode;
  switch(aFrame->Format)
    {
    case ePvFmtMono8: 	mode = Y8;		break;
    case ePvFmtMono16: 	mode = Y16;		break;
    case ePvFmtBayer8: 	mode = BAYER_RG8;	break;
    case ePvFmtBayer16: mode = BAYER_RG16;	break;
    case ePvFmtRgb24:   mode = RGB24;           break;
    case ePvFmtBgr24:   mode = BGR24;           break;
    default:
      DEB_ERROR() << "Format not supported: " << DEB_VAR1(aFrame->Format);
      m_sync->stopAcq();
      return;
    }

  m_continue_acq =  m_video->callNewImage((char*)aFrame->ImageBuffer,
					  aFrame->Width,
					  aFrame->Height,
					  mode);
  if(stopAcq || !m_continue_acq)
    m_sync->stopAcq(false);
}

//-----------------------------------------------------
// @brief range the binning to the maximum allowed
//-----------------------------------------------------
void Camera::checkBin(Bin &hw_bin)
{
    DEB_MEMBER_FUNCT();


    int x = hw_bin.getX();
    if(x > m_maxwidth)
        x = m_maxwidth;

    int y = hw_bin.getY();
    if(y > m_maxheight)
        y = m_maxheight;

    hw_bin = Bin(x,y);
    DEB_RETURN() << DEB_VAR1(hw_bin);
}
//-----------------------------------------------------
// @brief set the new binning mode
//-----------------------------------------------------
void Camera::setBin(const Bin &set_bin)
{
    DEB_MEMBER_FUNCT();

    PvAttrUint32Set(m_handle, "BinningX", set_bin.getX());
    PvAttrUint32Set(m_handle, "BinningY", set_bin.getY());

    m_bin = set_bin;
    
    DEB_RETURN() << DEB_VAR1(set_bin);
}

//-----------------------------------------------------
// @brief return the current binning mode
//-----------------------------------------------------
void Camera::getBin(Bin &hw_bin)
{
    DEB_MEMBER_FUNCT(); 

    tPvUint32 xValue; 
    tPvUint32 yValue;  

    PvAttrUint32Get(m_handle,"BinningX",&xValue); 
    PvAttrUint32Get(m_handle,"BinningY",&yValue);

    Bin tmp_bin(xValue, yValue);
    
    hw_bin = tmp_bin;
    m_bin = tmp_bin;
    
    DEB_RETURN() << DEB_VAR1(hw_bin);
}
