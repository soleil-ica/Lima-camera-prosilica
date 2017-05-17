.. _camera-prosilica:

Prosilica
---------

.. image:: prosilica.jpg

Introduction
````````````

AVT offers a large choice of FireWire and GigE cameras for machine vision, computer vision and other industrial or medical applications. Cameras by AVT and Prosilica include sensitive machine vision sensors (CCD and CMOS, VGA to 16 Megapixels) and fit a wide range of applications.

The Lima module as been tested with color and B/W GigE camera.

Installation & Module configuration
````````````````````````````````````

-  follow first the steps for the linux installation :ref:`linux_installation`

The minimum configuration file is *config.inc* :

.. code-block:: sh

  COMPILE_CORE=1
  COMPILE_SIMULATOR=0
  COMPILE_SPS_IMAGE=1
  COMPILE_ESPIA=0
  COMPILE_FRELON=0
  COMPILE_MAXIPIX=0
  COMPILE_PILATUS=0
  COMPILE_BASLER=0
  COMPILE_PROSILICA=1
  COMPILE_CBF_SAVING=0
  export COMPILE_CORE COMPILE_SPS_IMAGE COMPILE_SIMULATOR \
         COMPILE_ESPIA COMPILE_FRELON COMPILE_MAXIPIX COMPILE_PILATUS \
         COMPILE_BASLER COMPILE_PROSILICA COMPILE_CBF_SAVING

  start the compilation :ref:`linux_compilation`





-  finally for the Tango server installation :ref:`tango_installation`

Initialisation and Capabilities
````````````````````````````````

Camera initialisation
......................

The camera will be initialized   by creating a Prosilica::Camera object.  The Camera contructor
sets the camera with default parameters, only the ip address or hostname of the camera is mandatory.

Std capabilites
................

This plugin has been implement in respect of the mandatory capabilites but with some limitations which
are due to the camera and SDK features. Only restriction on capabilites are documented here.

* HwDetInfo
  
  getCurrImageType/getDefImageType(): it can change if the video mode change (see HwVideo capability).

  setCurrImageType(): It only supports Bpp8 and Bpp16.

* HwSync

  get/setTrigMode(): the only supported mode are IntTrig, IntTrigMult and ExtTrigMult.
  
  

Optional capabilites
........................
In addition to the standard capabilities, we make the choice to implement some optional capabilities which
are supported by the SDK. Video and Binning are available.

* HwVideo

  The prosilica cameras are pure video device, so video format for image are supported:
   
  **Color cameras ONLY** 
   - BAYER_RG8
   - BAYER_RG16
   - RGB24
   - BGR24
   
  **Color and Monochrome cameras**
   - Y8   

  Use get/setMode() methods of the *video* object (i.e. CtControl::video()) to read or set the format.

* HwBin 

  There is no restriction for the binning up to the maximum size.

Configuration
``````````````

- First you have to setup ip address of the Prosilica Camera by using *CLIpConfig* (camera/prosilica/sdk/CLIpConfig)
- list of all cameras available : *CLIpConfig -l* (If you do not see any camera, that bad news!)
- finally set ip add : *CLIpConfig -u UNIQUE_NUMBER -s -i 169.254.X.X -n 255.255.255.0 -m FIXED* (It's an example!)
- Then in the Prosilica Tango device set the property *cam_ip_address* to the address previously set.

That's all....

How to use
````````````
This is a python code example for a simple test:

.. code-block:: python

  from Lima import Prosilica
  from lima import Core

  cam = Prosilica.Camera("192.169.1.1")

  hwint = Prosilica.Interface(cam)
  ct = Core.CtControl(hwint)

  acq = ct.acquisition()

  # set video  and test video

  video=ct.video()
  video.setMode(Core.RGB24)
  video.startLive()
  video.stopLive()
  video_img = video.getLastImage()

  # set and test acquisition 

  # setting new file parameters and autosaving mode
  saving=ct.saving()

  pars=saving.getParameters()
  pars.directory='/buffer/lcb18012/opisg/test_lima'
  pars.prefix='test1_'
  pars.suffix='.edf'
  pars.fileFormat=Core.CtSaving.TIFF
  pars.savingMode=Core.CtSaving.AutoFrame
  saving.setParameters(pars)

  acq.setAcqExpoTime(0.1)
  acq.setNbImages(10) 
  ct.prepareAcq()
  ct.startAcq()

  # wait for last image (#9) ready
  lastimg = ct.getStatus().ImageCounters.LastImageReady
  while lastimg !=9:
    time.sleep(0.01)
    lastimg = ct.getStatus().ImageCounters.LastImageReady
 
  # read the first image
  im0 = ct.ReadImage(0)
