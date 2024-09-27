==================
Multi-Mode keyboard
==================

.. contents::
   :depth: 2

概述
=======

本文档旨在给用户介绍基于Realtek Zephyr SDK和RTL87x2G系列soc开发多模键盘的方案。键盘目前使用的是zmk架构，zmk全称是Zephyr Mechanical Keyboard, 具有较为完备的keyboard功能，它由开源社区开发和维护。

关于zephyr的介绍可参考：`https://docs.zephyrproject.org/latest <https://docs.zephyrproject.org/latest/>`_

关于zmk的介绍可参考：`https://zmk.dev <https://zmk.dev/>`_

支持功能
-----------

-  支持BLE/2.4G/USB三种通信模式
-  BLE/2.4G/USB三种模式的最高上报率为125Hz/4KHz/8KHz
-  支持Full/High speed USB
-  支持GPIO和Keyscan按键，Keyscan最多可扩展到240个按键(12x20)
-  支持PWM输出控制LED
-  支持电量检测
-  支持基于USB的DFU升级

Zephyr SDK 准备
-----------

.. note:: 
   - 本文档介绍如何在主机上交叉编译和生成 zephyr 二进制映像。请注意，当前的 Realtek Zephyr SDK 环境基于 Windows 10。因此，接下来的指南将基于 Windows。对于 MacOS 和 Linux 用户，请参阅 Zephyr 官方文档。

1. 参考zephyr官方的文档 `https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies <https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies/>`_，安装相关的依赖。

需要确保下面表格中列出的工具均已安装成功：

.. csv-table::
   :header: 工具名, 最低版本
   :widths: 15 15
   :align: center

   CMake, 3.30.5
   Python, 3.8
   Devicetree compiler, 1.4.6
   Zephyr sdk bundle, 0.16.8
   Chocolatery, /
   west, /
   Ninja, /
   Gperf, /
   Git, /
   Wget, /
   7zip, /

2. 下载Realtek Zephyr SDK

Realtek Zephyr SDK总共包含四个repo `https://github.com/rtkconnectivity <https://github.com/rtkconnectivity/>`_

   **zephyr:** 基于 `https://github.com/zephyrproject-rtos/zephyr <https://github.com/zephyrproject-rtos/zephyr/>`_ 以及Realtek的适配

   **hal_realtek:** Realtek soc 的hal模块

   **zmk:** 基于 `https://github.com/zmkfirmware/zmk <https://github.com/zmkfirmware/zmk/>`_ 以及Realtek的适配

   **realtek-zephyr-project:** 存放基于zephyr os实现的Realtek的applications

获取Realtek Zephyr SDK, 打开windows自带的cmd.exe, 参考以下命令

通过git clone命令来拉取ZMK repo
.. code-block:: c

  git clone --branch zmk_rtk https://github.com/rtkconnectivity/zmk.git

需要把zmk/app作为manifest repo workplace

.. code-block::

  west init -l zmk/app 

//拉取zmk编译所需要的zephyr, hal_realtek等其他repos

.. code-block:: c

  west update 

基于board:rtl8762gn_evb，查看编译的配置选项

.. code-block:: c

  cd zmk/app

  west build -p -b rtl8762gn_evb -t menuconfig -- -DSHIELD=rtk_keyboard 

Zephyr通过多层级的Kconfig机制来管理内核，驱动层以及应用层所需的配置，用户可以通过menuconfig在线修改相关配置

.. figure:: ../figures/zmk_menuconfig.*
   :align: center
   :scale: 50%
   :name: zmk编译配置选项

   zmk编译配置选项

编译示例工程，编译出的zmk.bin以及zmk.hex在路径build/zephyr下

.. code-block:: c

  west build -b rtl8762gn_evb -- -DSHIELD=rtk_keyboard 

3. 

环境需求
==========

.. tabs::

    .. tab:: 硬件环境需求

          1. RTL87x2G EVB（母板+子板）
          2. J-link
          3. FT232_serial_port_board

    .. tab:: 工具需求

          1. BeeMPTool_kits: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\MPTool`
          2. DebugAnalyzer: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\DebugAnalyzer`
          3. CFUDownloadTool: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\CFUDownloadTool`
          4. MPPackTool: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\BeeMPTool\\BeeMPTool\\tools\\MPPackTool`
          5. Serial Terminal tool(such as mobaxterm)

工具需求说明
--------------

在SDK的 :file:`\\tools` 目录下，安装包是.zip格式的压缩包，用户需要对其解压。串口工具需要用户自行下载，用于显示zephyr log

下载固件到IC
~~~~~~~~~~~~~~~

除了上文编译出的zmk.bin和zmk.hex, RTL87x2G还需要烧录以下images，

.. csv-table::
   :header: Image名, 描述， 文件位置
   :widths: 15 30 20
   :align: center

   System Config file，记录有关硬件配置和蓝牙配置的信息，如配置 BT 地址、更改链路数等。配置文件可使用 MPTool 生成，/
   OTA Header file，定义了flash布局，由MPTool生成，/
   BOOT Patch Image，Realtek发布，ROM code中保留了Patch函数入口，通过Patch可以优化Boot flow、修改secure ROM code原有行为、扩充ROM code功能等。，/
   System Patch Image, Realtek发布，ROM code中保留了Patch函数入口，通过Patch可以修改non-secure ROM code原有行为、扩充ROM code功能等，/
   BT Stack Patch Image，Realtek发布，通过Patch可扩充ROM code功能，增加BT Controller中stack相关feature的支持，/

对于zephyr app image, 有两种烧录方式，一种是通过MPTool烧录，一种是通过 “west flash” 烧录。

- MPtool烧录

   1. 可以将编译出的zmk.bin作为user data的形式烧录到app对应的地址，地址需要和flash map中保持一致，flash map可参考

- west flash烧录

MP Tool
~~~~~~~~~~

用户烧录程序请参考 :ref:`MPTool 下载` 章节，烧录文件路径为 :file:`BEE4-SDK-MOUSE-vx.x.x\\download_images`。

.. note:: 
   - 当选择Config File时，注意文件前缀：log_close表示设备在工作模式没有log输出，log_open表示在工作模式下设备的log会同步输出，如 \ :ref:`图片-Config File Log 前缀`\  所示。
   - 有关MP Tool的更多使用说明，请参阅SDK工具目录下的用户指南，也可以前往 `RealMCU <https://www.realmcu.com/en/Home/DownloadList/c175760b-088e-43d9-86da-1fc9b3f07ec3>`_ 平台获取相应工具，并查阅提供的文档。

.. figure:: ../figures/ConfigFile_Log_prefix.*
   :align: center
   :name: 图片-Config File Log 前缀

   Config File Log 前缀

.. _tri-mode mouse DebugAnalyzer:

DebugAnalyzer
~~~~~~~~~~~~~~~~

用户抓取和解析SoC Log请参考 :ref:`DebugAnalyzer介绍 <Debug Analyzer Tool CN>`。

.. note:: 
   - 确保.trace文件与当前SoC运行代码匹配。开发过程中如遇到问题，请提供 :file:`DebugAnalyzer\\DataFile` 路径下的 **.log** & **.bin** & **.cfa** 文件以及 **.trace** 文件，以便Realtek解析定位问题。
   - 有关DebugAnalyzer的更多使用说明，请参阅SDK工具目录下的用户指南，也可以前往 `RealMCU <https://www.realmcu.com/en/Home/DownloadList/c175760b-088e-43d9-86da-1fc9b3f07ec3>`_ 平台获取相应工具，并查阅提供的文档。

.. _tri-mode mouse MPPack Tool:

MPPack Tool
~~~~~~~~~~~~~~

1. | 用户可以通过MPPackTool对设备升级文件进行打包处理。
   | 路径: :file:`BEE4-SDK-MOUSE-vx.x.x \\tools\\BeeMPTool\\BeeMPTool\\tools\\MPPackTool`，如下图所示。

.. figure:: ../figures/MPPackTool.exe.*
   :align: center
   :name: MPPackTool.exe CN

   MPPackTool.exe

2. 双击运行 :file:`MPPackTool.exe`， :guilabel:`IC Type` 选择 :guilabel:`RTL87x2G_VB`，选择 :guilabel:`ForCFU`，点击 :guilabel:`Browse`，选择需要升级的文件，以 :file:`Bank0 Boot Patch Image` 和 :file:`Bank0 BT Stack Patch Image` 为例，如下图所示。

.. figure:: ../figures/MPPackTool_Interface.*
   :align: center
   :scale: 70%
   :name: MPPackTool界面

   MPPackTool界面

.. hint:: 
   待升级的文件大小不能超过设置的OTA Tmp区大小（参考工程里的 :file:`flash_map.h`），如果待升级总文件的大小超出OTA Tmp区，需要分批次打包，然后逐包升级。由于Patch，Stack等文件很少更新，用户一般只需要单独打包APP Image即可。

3. | 文件加载完成后，点击 :guilabel:`Confirm`，会生成 :file:`ImaPacketFile.offer.bin` 和 :file:`ImgPacketFile.payload.bin` 两份文件，如下图所示。
   | 路径: :file:`BEE4-SDK-MOUSE-vx.x.x\\tools\\BeeMPTool_x.x.x.x\\BeeMPTool\\tools`

.. figure:: ../figures/MPPackTool_File_packaging_confirmation.*
   :align: center
   :scale: 70%
   :name: MPPackTool文件打包确认

   MPPackTool文件打包确认

.. note:: 
   - 勾选 :guilabel:`save patch`，可以选择生成CFU文件的保存目录，不勾选默认存在根目录下。
   - 打包量产烧录文件以及其他更详细的使用说明，请参阅SDK工具目录下的用户指南，也可以前往 `RealMCU <https://www.realmcu.com/en/Home/DownloadList/c175760b-088e-43d9-86da-1fc9b3f07ec3>`_ 平台获取相应工具，并查阅提供的文档。

.. _tri-mode mouse CFUDownloadTool:

CFUDownloadTool
~~~~~~~~~~~~~~~~~~

1. | 用户可以通过CFUDownloadTool对设备进行程序升级。
   | 路径: :file:`BEE4-SDK-MOUSE-vx.x.x\\tools\\CFUDownloadTool`，软件版本不低于V2.0.2.0，如下图所示。

.. figure:: ../figures/CFUDownloadTool.*
   :align: center
   :name: CFUDownloadTool CN

   CFUDownloadTool

2. 打开 :file:`CFUTOOLSettings.ini` 文件，对升级设备进行参数设置，如下所示。

   1. RTL87x2G升级方式采用CFU_VIA_USB_HID
   2. Mouse：Vid=0x0BDA，Pid=0x4762
   3. Dongle：Vid=0x0BDA，Pid=0x4762

.. highlight:: rst

::

   [CFU_VIA_USB_HID]
   Vid=0x0bda
   Pid=0x4762
   UsagePage=0xff0b
   UsageTlc=0x0104

   [CFU_EARBUD_VIA_BT_HID]
   Vid=0x005d
   Pid=
   UsagePage=0xff0b
   UsageTlc=0x0104

   [CFU_EARBUD_VIA_DONGLE]
   Vid=0x0bda
   Pid=0x4762
   UsagePage=0xff07
   UsageTlc=0x0212

   [ICTypeSelect]
   TYPE=1

   [CFUTypeSelect]
   Type=0

   [MainSetting]
   ImageDir=BEE4-SDK-MOUSE\applications\trimode_mouse\proj\mdk\images\app\cfu
   TransDelay=0
   TransTimeout=200
   ForceReset=1

   [DEVICE]
   SerialNumber=

.. highlight:: none

.. hint:: 
   - 如果配置的Vid和Pid与Mouse/Dongle设置不一致，CFUDownloadTool会识别不到设备。
   - TransDelay可设置两笔数据包之间的延迟时间。
   - TransTimeout可设置response超时时间。
   - 通过设置SerialNumber，可以在VID、PID相同时，区分升级的是dongle还是mouse，如果不填写，则代表不区分。

3. 双击运行 :guilabel:`CFUDownloadTool.exe`，如下图所示。

   - :guilabel:`IC Type` 选择: :guilabel:`RTL87x2G`
   - :guilabel:`CFU Type` 选择: :guilabel:`CFU via USB HID`

.. figure:: ../figures/CFU_Download_Tool_interface.*
   :align: center
   :scale: 80%
   :name: CFU Download Tool界面

   CFU Download Tool界面

4. 将设备与电脑连接，如果设备识别成功，如下图所示。

   1. “Found 1 device”在界面右侧显示。
   2. “FwVersion”表示当前Mouse/Dongle的App Image版本。
   3. “Current Bank 2”表示Single Bank升级方案（当前仅支持此方案）。

.. figure:: ../figures/CFU_Download_Tool_Device_identification_interface.*
   :align: center
   :scale: 70%
   :name: 图片-CFU Download Tool设备识别界面

   CFU Download Tool设备识别界面

5. 在 :guilabel:`CFU Image` 处加载需要升级的文件所在文件夹，如 \ :ref:`图片-CFU Download Tool设备识别界面`\  和 \ :ref:`图片-CFU文件`\  所示。

.. figure:: ../figures/CFU_Files.*
   :align: center
   :name: 图片-CFU文件

   CFU文件

6. 点击 :guilabel:`Download`，进度条会显示当前程序下载进度，下载完成显示“OK”，如 \ :ref:`图片-CFU文件下载成功`\  所示。升级完成后，点击 :guilabel:`Get Device`，右侧FwVersion会显示当前Image版本，确保升级成功。

.. figure:: ../figures/CFU_Files_download_succeed.*
   :align: center
   :scale: 60%
   :name: 图片-CFU文件下载成功

   CFU文件下载成功

硬件连线
===========

.. _tri-mode mouse RTL87x2G EVB:

RTL87x2G EVB
---------------

EVB评估板提供了用户开发和应用调试的硬件环境。EVB由主板和子板组成。它有下载模式和工作模式，具体使用请参考 \ :Doc:`快速入门 <../../../../doc/quick_start/text_cn/README>`\  的硬件开发环境这一章节。

RTL87x2G 三模鼠标
--------------------

.. figure:: ../figures/Tri-Mode_Mouse_Device.*
   :align: center
   :scale: 30%
   :name: 三模鼠标样机引线

   三模鼠标样机引线

下载模式
~~~~~~~~~~~

1. 设备上电前，用户需要先将引出的Log Pin接地，设备的Log Pin、GND Pin和 \ :ref:`FT232串口转接板`\  的GND要共地；引出的Tx接串口转接板的Rx，Rx接串口转接板的Tx；VBAT接3.3V电源，串口转接板连接PC进行供电。

   .. figure:: ../figures/FT232_serial_port_board.*
      :align: center
      :scale: 35%
      :name: FT232串口转接板

      FT232串口转接板

2. 进入下载模式后，请参照 :ref:`tri-mode mouse MP Tool` 进行程序烧录。需注意：

   1. 芯片在上电后会读取Log Pin的电平信号，如果电平为低，则Bypass Flash，进入烧录模式，否则运行应用层程序。
   2. 因为芯片烧录需要使用1M波特率，务必使用FT232的串口转接板，否则可能会出现UART Open Fail的现象。

Log接线
~~~~~~~~~~

设备引出来的Log Pin连接串口转接板的Rx，GND连接串口转接板的GND，电脑需要和设备连接进行供电，连接完成后，请参照 :ref:`tri-mode mouse DebugAnalyzer` 查看Log输出。

配置选项
===========

SDK中tri-mode mouse应用默认的主要配置如下：

1. FEATURE_RAM_CODE（开）
   配置是否将所有代码复制到RAM中运行。
2. FEAUTRE_SUPPORT_FLASH_2_BIT_MODE（关）
   配置是否跑flash 2 bit mode。
3. FEATURE_SUPPORT_NO_ACTION_DISCONN（开）
   配置是否使能无操作断线机制。
4. FEATURE_SUPPORT_AUTO_PAIR_WHEN_POWER_ON（关）
   配置mouse上电时是否自动触发配对。
5. FEATURE_SUPPORT_APP_ACTIVE_FTL_GC（开）
   配置是否允许APP主动触发 :term:`FTL` 垃圾回收
6. FEATURE_SUPPORT_AUTO_TEST（关）
   配置是否使能自动测试。
7. ENABLE_2_4G_LOG（关）
   配置是否打开2.4g stack log。
8. 各module的其他主要配置如下，将在后续章节具体说明。

.. code-block:: c

   #define MOUSE_GPIO_BUTTON_EN               1
   #define MOUSE_KEYSCAN_EN                   0
   #define MODE_MONITOR_EN                    1
   #define PAW3395_SENSOR_EN                  1
   #define AON_QDEC_EN                        1
   #define GPIO_QDEC_EN                       0
   #define SUPPORT_LED_INDICATION_FEATURE     1
   #define LED_FOR_TEST                       0
   #define SUPPORT_BAT_DETECT_FEATURE         1
   #define DLPS_EN                            1

编译和下载
============

请参阅 :ref:`快速入门-编译和下载 <编译和下载>` 进行编译和下载。

生成Flash Map
----------------

在 :ref:`生成Flash Map` 步骤中，开发人员需要根据 :file:`BEE4-SDK-MOUSE-vx.x.x\\applications\\trimode_mouse\\proj\\flash_map.h` 生成 :file:`flash_map.ini` 和对应的OTA Header。

生成System Config File
-------------------------

在 :ref:`生成System Config File` 步骤中，tx power等其他设定可根据需要进行配置。

编译APP image
----------------

在 :ref:`编译APP image` 步骤中，tri-mode mouse SDK keil工程的路径为: :file:`BEE4-SDK-MOUSE-vx.x.x\\applications\\trimode_mouse\\proj\\mdk`。APP image由该工程编译得到。

KEIL编译
~~~~~~~~~~~

1. 工程共有2个target，使用不同的2.4G专属协议，请选择正确的target进行编译。默认编译不带“_hopping”后缀的target，遵循sync协议，具体可参考 :Doc:`2.4G协议文档 <../../../../subsys/ppt/doc/text_cn/README>`；而带有“_hopping”后缀的target使用sync5协议通信，可实现扫描波段、跳频功能。

.. note:: 
   如果需要使用“跳频-双8K”方案，鼠标和dongle都要切换到hopping target，如下图所示。如果只切换一个target，则无法完成2.4G配对。

.. figure:: ../figures/Mouse_Target_switchover.*
   :align: center
   :scale: 70%
   :name: 鼠标target切换

   鼠标target切换

.. figure:: ../figures/Dongle_Target_switchover.*
   :align: center
   :scale: 70%
   :name: Dongle target切换

   Dongle target切换

.. csv-table:: application和protocol的位置
   :header: 名称, 位置
   :widths: 20 60
   :align: center

   Mouse application, BEE4-SDK-MOUSE-vx.x.x\\applications\\trimode_mouse
   Dongle application, BEE4-SDK-MOUSE-vx.x.x\\applications\\ppt_dongle
   2.4G sync protocol, BEE4-SDK-MOUSE-vx.x.x\\subsys\\ppt\\sync
   2.4G sync5 protocol, BEE4-SDK-MOUSE-vx.x.x\\subsys\\ppt\\sync5

2. 工程编译成功，在\\bin文件夹下会同步生成一份带MP前缀的.bin文件及对应的.trace文件，如下图所示，用户可以通过MPTool烧录APP Image，在DebugAnalyzer加载.trace文件解析Log。

.. figure:: ../figures/Project_compile_generate_file.*
   :align: center
   :scale: 70%
   :name: 工程编译生成文件

   工程编译生成文件

GCC编译
~~~~~~~~~~

1. GCC编译前，需要参照 \ :ref:`快速入门-GCC<GCC CN>`\  这一章节进行正确的环境配置。

2. 完成环境配置后，访问mingw64\\bin，复制mingw32-make.exe，并将复制的文件重命名为“make.exe”。

3. 用户使用GCC编译时，需在Makefile文件的位置打开bash来执行make命令。Makefile的路径如下：

   - Dongle工程: :file:`BEE4-SDK-MOUSE-vx.x.x\\applications\\ppt_dongle\\proj\\gcc`
   - 三模鼠标工程: :file:`BEE4-SDK-MOUSE-vx.x.x\\applications\\trimode_mouse\\proj\\gcc`

   .. figure:: ../figures/Location_of_Makefile.*
      :align: center
      :scale: 60%
      :name: Makefile路径

      Makefile路径

4. | 对于hopping工程，用户应在命令行中的 :mod:`make` 命令后定义 :mod:`ppt_transport=enable`。若用户想要生成"非hopping"的image，只需在命令行中输入make即可。编译dongle_hopping工程和trimode_mouse_hopping工程的完整命令如下：

   .. code-block:: c

      make ppt_transport=enable

   | 编译完成后，\\bin文件夹中会生成一个带有MP前缀的APP image bin文件和相应的APP trace文件。与KEIL编译的image不同，GCC生成的image文件名中带有“hopping”字样的图像。\\bin文件夹的内容如下图所示。

   .. figure:: ../figures/Location_of_APP_image_bin.*
      :align: center
      :scale: 60%
      :name: APP image bin路径

      APP image bin路径

   | 如果用户想重新编译APP image，请确保先执行 :mod:`make clean` 命令。
   
   | 除了在命令行中输入命令外，用户还可以在gcc文件夹下直接执行shell脚本 :file:`build_all_target.sh`，\\bin文件夹会同时生成两个target的image和相关文件。\\bin的内容如下图所示。

   .. code-block:: c

      ./build_all_target.sh

   .. figure:: ../figures/Contents_after_the_compliation_of_build_all_target.sh.*
      :align: center
      :scale: 60%
      :name: shell脚本执行后的bin目录

      shell脚本执行后的bin目录

文件下载
-----------

MP Tool下载
~~~~~~~~~~~~~~

请参照 :ref:`tri-mode mouse MP Tool` 进行文件下载。

J-Link下载
~~~~~~~~~~~~~

J-Link支持多种连接接口，如JTAG、SWD等，因为SWD使用的接线更少，所以RTL87x2G采用这种接口: :ref:`RTL87x2G(BEE4) - SWD对应接口`。另外，J-Link也可以与多种开发环境和IDE（如Keil MDK、IAR Embedded Workbench等）兼容，Keil环境设置可以参考 :Doc:`平台概述 <../../../../doc/platform/platform_overview/text_cn/README>`。

建议使用J-Link Software v6.44（或更新）版本，更多信息可以参考 :Doc:`快速入门 <../../../../doc/quick_start/text_cn/README>` 编译和调试。

.. csv-table:: RTL87x2G(BEE4)-SWD对应接口
  :header: RTL87x2G, SWD
  :widths: 50 50
  :align: center  
  :name: RTL87x2G(BEE4) - SWD对应接口

  GND, GND
  P1_0, SWIO
  P1_1, SWCK
  VDDIO, Vterf/3.3V

测试验证
===========

代码修改
-----------

由于代码里的某些功能实现和鼠标样机直接挂钩，例如：LED显示/按键操作/滚轮操作等。如果将APP Image烧录到鼠标样机，用户可直接进行编译和烧录；如果烧录到EVB（非鼠标样机），为保证程序正常运行，这边以2.4G模式为例，请参考以下修改说明：

1. Mouse端: :file:`BEE4-SDK-MOUSE-vx.x.x\\applications\\trimode_mouse\\proj\\mdk`

   1. 在 :file:`board.h` 中修改以下定义。

      .. code-block:: c

         #define MOUSE_HW_SEL            MOUSE_6_KEYS

   2. 通过修改 :file:`board.h` 中的 **AUTO_TEST_USE_ROUND_DATA** ，决定画方还是画圆，设定为1代表画圆。

      .. code-block:: c

         #define AUTO_TEST_USE_ROUND_DATA       1

2. Dongle端: :file:`BEE4-SDK-MOUSE-vx.x.x\\applications\\ppt_dongle\\proj\\mdk`。

   - 程序不做修改，上电后默认开启配对。
   - 程序编译通过后，请参照 :ref:`tri-mode mouse RTL87x2G EVB` 确认硬件环境，参照 :ref:`tri-mode mouse MP Tool` 这一小节进行程序烧录。

3. Mouse端Log显示 **"SYNC_EVENT_CONNECTED"** 代表已经成功连接，如下所示。

.. highlight:: rst

::

   0000233  07-25#11:12:35.018  251  0009287.531  [APP] !**[ppt_app_sync_event_cb] SYNC_EVENT_PAIRED
   0000234  07-25#11:12:35.018  252  0009287.531  [APP] !**[ppt_app_sync_event_cb] SYNC_EVENT_CONNECTED

.. highlight:: none

Log抓取和分析
---------------

用户可以通过AnalyzerDebug来查看Log判断程序是否正常运行，请参照 :ref:`tri-mode mouse DebugAnalyzer` 这一小节查看Log输出。以下对三种模式下的关键log作简要说明。

1. BLE模式：

   - log显示 **"GAP stack ready"** 代表GAP层已完成初始化；
   - 搜索到 **"GAP adv start"** 代表已开始发广播，可以根据 **"mouse_start_adv"** 相关log查看当前发的广播类型；
   - 搜索 **"GAP_AUTHEN_STATE_COMPLETE"** 相关log查看当前配对结果是成功/失败。

2. 2.4G模式：

   - log显示 **"ppt_pair"** 代表开始配对；
   - **"ppt_reconnect"** 代表处于回连状态；
   - **"SYNC_EVENT_CONNECTED"** 代表已经成功连接。

3. USB模式：

   - log显示 **"[app_usb_state_change_cb] state: 5"** 代表USB枚举成功；
   - **"[app_usb_speed_cb] speed:"** 相关log代表当前USB speed，0表示Full Speed，1表示High Speed。

软件设计介绍
==============

本章主要介绍RTL87x2G Tri-Mode Mouse解决方案的软件相关技术参数和行为规范，为Tri-Mode Mouse的所有功能提供软件概述，包括三种模式、按键、滚轮、光学传感器、电量检测和充电、灯效、产测等行为规范，用于指导Tri-Mode Mouse的开发和追踪软件测试中遇到的问题。

源代码目录
-------------

* 工程文件目录： :file:`sdk\\applications\\trimode_mouse\\proj`
* 源代码目录： :file:`sdk\\applications\\trimode_mouse\\src`

Tri-Mode Mouse application中的源文件目前分为以下几类：

.. highlight:: rst

::

   └── Project: trimode_mouse
       ├── include             
       └── Device                                  includes startup code
           ├── startup_rtl.c
           └── system_rtl.c
       ├── CMSE Library                            Non-secure callable lib
       ├── Lib                                     includes all binary symbol files that user application is built on
       ├── Peripheral                              includes all peripheral drivers and module code used by the application
       └── APP                                     includes the tri-mode mouse user application implementation
           ├── main.c
           ├── app_task.c
           ├── mouse_application.c
           ├── mouse_ppt_app.c
           ├── swtimer.c
           ├── loop_queue.c
           └── mouse_ppt_trans_handle.c            only compiled in hopping targets
       └── ble                                     includes BLE services and the tri-mode mouse bluetooth app
           ├── bas.c
           ├── dis.c
           ├── hid_ms.c
           ├── privacy_mgnt.c
           └── mouse_gap.c
       └── ppt                                     includes 2.4G module interfaces for application
           └── ppt_sync_app.c
       └── ppt_trans                               includes 2.4G transport layer interfaces to application and only compiled in hopping targets
       └── usb                                     includes usb module settings for tri-mode mouse application
           ├── usb_device.c
           ├── usb_hid_interface_mouse.c
           ├── usb_hid_interface_keyboard.c
           ├── usb_hid_interface_dfu.c
           └── usb_handle.c
       └── mode_monitor                            includes the implementation of tri-mode mouse mode monitor module
           ├── mode_monitor_driver.c
           └── mode_monitor_handle.c
       └── mouse_button                            includes button module files implemented by gpio and keyscan
           ├── mouse_gpio_button_driver.c
           ├── mouse_keyscan_driver.c
           ├── mouse_button_handle.c
           └── mouse_button_sw_debounce_handle.c
       └── paw3395                                 includes the implementation of paw3395 sensor module
           ├── paw3395_driver.c
           └── paw3395_handle.c
       └── qdec                                    includes the implementation of qdec module
           ├── qdec_driver.c
           ├── gpio_qdec_driver.c
           └── qdec_handle.c
       └── led                                     includes led module files implemented by gpio and hardware timer
           ├── led_gpio_ctl_driver.c
           ├── led_hw_tim_pwm_driver.c
           └── led_driver.c
       └── battery                                 includes battery module interfaces to tri-mode mouse
           └── battery_driver.c
       └── dfu                                     includes the implementation of usb dfu protocol
           ├── usb_dfu.c
           └── dfu_common.c
       └── mp_test                                 includes mp test module interfaces to tri-mode mouse
           ├── hci_transport_if.c
           ├── rf_test_mode.c
           ├── mp_test.c
           └── single_tone.c

.. highlight:: none

Flash布局
------------

应用程序默认的flash布局头文件： :file:`sdk\\applications\\trimode_mouse\\proj\\flash_map.h`。

.. csv-table:: Flash布局
   :header: Example layout with a total flash size of 1MB,Size(byte),Start Address
   :widths: 100 50 50
 
   Reserved, 4K, 0x04000000
   OEM Header, 4K, 0x04001000
   Bank0 Boot Patch, 32K, 0x04002000
   Bank1 Boot Patch, 32K, 0x0400A000
   OTA Bank0, 620K, 0x04012000
   - OTA Header, 4K, 0x04012000
   - System Patch code, 32K, 0x04013000
   - BT Lowerstack Patch code, 60K, 0x0401B000
   - BT Host code, 212K, 0x0402A000
   - APP code, 308K, 0x0405F000
   - APP Config File, 4K, 0x040AC000
   - APP data1, 0K, 0x040AD000
   - APP data2, 0K, 0x040AD000
   - APP data3, 0K, 0x040AD000
   - APP data4, 0K, 0x040AD000
   - APP data5, 0K, 0x040AD000
   - APP data6, 0K, 0x040AD000
   OTA Bank1, 0K, 0x040AD000
   Bank0 Secure APP code, 0K, 0x040AD000
   Bank0 Secure APP Data, 0K, 0x040AD000
   Bank1 Secure APP code, 0K, 0x040AD000
   Bank1 Secure APP Data, 0K, 0x040AD000
   OTA Temp, 312K, 0x040AD000
   FTL, 16K, 0x040FB000
   APP Defined Section1, 4K, 0x040FF000
   APP Defined Section2, 0K, 0x04100000

.. important::
  - 如需调整Flash布局，请参考 :Doc:`快速入门 <../../../../doc/quick_start/text_cn/README>` 中 :ref:`生成Flash Map` 的步骤。
  - 调整Flash布局后，必须使用新的 :file:`flash_map.ini` 重新进行 :ref:`生成OTA header` 和 :ref:`生成System Config File` 步骤。
  - 调整Flash布局后，必须使用新的 :file:`flash_map.h` 替换 :file:`sdk\\applications\\trimode_mouse\\proj\\flash_map.h` 重新进行 :ref:`编译APP image` 步骤。

软件架构
-----------

系统软件架构如下图所示。

.. figure:: ../figures/mouse_software_architecture.*
   :align: center
   :scale: 70%
   :name: Tri-Mode Mouse软件架构图

* **Platform**: 包括OTA、Flash、:term:`FTL` 等。
* **IO Drivers**: 提供对RTL87x2G外设接口的应用层访问。
* **OSIF**: 实时操作系统的抽象层。
* **GAP**: 用户应用程序与BLE协议栈通信的抽象层。

任务和优先级
--------------

如下图所示，应用程序共创建了六个任务：

.. figure:: ../figures/task_and_priority.*
   :align: center
   :scale: 80%

   Tasks

各任务描述及优先级如下表：

.. csv-table::
   :header: 任务, 描述, 优先级
   :widths: 15 30 10
   :align: center
  
   Timer, 实现FreeRTOS所需的软件定时器, 6
   BT Controller stack, 实现 :term:`HCI` 以下的BLE协议栈, 6
   BT Host stack, 实现 :term:`HCI` 以上的BLE协议栈, 5
   USB, 处理USB数据交互, 3
   Application, 处理用户应用程序需求, 2
   Idle, 运行后台任务，包括 :term:`DLPS`, 0

.. note::
  - 可以创建多个应用任务，并相应地分配内存资源。
  - FreeRTOS提供Idle任务和Timer任务。
  - 已使用 SysTick 中断将任务配置为根据其优先级进行抢占。
  - 中断服务例程 (:term:`ISR`) 已由供应商实施。

程序初始化处理流程
--------------------

鼠标上电后，APP的初始化过程主要包含在 :func:`main` 函数和 :func:`app_main_task` 函数中，BLE/2.4G/USB三种模式下的初始化过程有一定差异。

Main函数
~~~~~~~~~~~

:func:`main` 函数中初始化过程包括：Flash模式设置，SWD设置，全局变量初始化，管脚初始化，驱动模块初始化，BLE/2.4G/USB三种模式相关的初始化内容，电源模式初始化，软件定时器初始化，看门狗初始化，以及app task初始化和开启任务调度。

.. figure:: ../figures/Initializations_in_main.*
   :align: center
   :scale: 80%
   :name: main函数初始化内容

   main函数初始化内容


.. list-table:: main初始化流程相关函数
   :header-rows: 1

   * - :term:`API`
     - 功能模块

   * - flash_nor_try_high_speed_mode()
     - 默认为1bit mode, FEAUTRE_SUPPORT_FLASH_2_BIT_MODE 设置为1后，会通过接口 :mod:`flash_nor_try_high_speed_mode(FLASH_NOR_IDX_SPIC0, FLASH_NOR_2_BIT_MODE)` 设置为2bit mode。2bit mode虽然操作flash速度更快，但也会增加静态功耗。因为大部分代码是跑在ram中的，且使用过程中操作flash场景和频次较少，所以推荐使用1bit mode即可。

   * - swd_pin_disable()
     - SWD可以在CPU active的时候作为debug手段，可以进行单步调试等，但需要使用P1_0，P1_1，需要将宏 SWD_ENABLE 设置为1。如果不使用SWD，或者需要使用P1_0或P1_1，需要设置宏 SWD_ENABLE 为0，会调用此接口 ，使得P1_0和P1_1不受影响。

   * - global_data_init()
     - 初始化所有模块所需要的全局变量。

   * - board_init()
     - 初始化各个外设模块的PAD设置和Pinmux设置。

   * - driver_init()
     - 初始化各个模块的驱动配置，包括判断和获取当前鼠标所处的通信模式是BLE/2.4G/USB中的哪一种。如果当前处于USB模式，最后需要初始化USB模块。

   * - Mode specific initialization
     - 如果处于BLE模式，需要对BLE进行相关的初始化，包括 :mod:`le_gap_init(1)`, :mod:`gap_lib_init()`, :mod:`app_le_gap_init()`, :mod:`app_le_profile_init()`。

   * - pwr_mgr_init()
     - 如果关闭DLPS_EN，或者当前处于USB模式，会设置为Active mode，并调用 :func:`pm_no_check_status_before_enter_wfi`，使得CPU不工作的时候能快速进入WFI，以降低功耗。BLE和USB模式在main函数中初始化电源模式，但2.4G模式需要在2.4G初始化之后才能进行电源模式初始化。

   * - sw_timer_init()
     - 初始化软件定时器。

   * - app_watchdog_open()
     - 打开看门狗，可以通过宏 WATCH_DOG_TIMEOUT_MS 设置看门狗超时复位的时间，默认是5秒。

   * - task_init()
     - 初始化app task。

   * - os_sched_start()
     - 开启任务调度。

app_main_task函数
~~~~~~~~~~~~~~~~~~~~

除了main函数包括的初始化过程外，在 :func:`main` 函数中创建的app task也包含了一部分初始化内容。当任务开始调度后，会跑到 :func:`app_main_task` , 完成任务堆栈的分配、任务消息队列的创建后，根据不同模式进行初始化。

   - 2.4G模式：2.4G初始化，电源模式初始化，2.4G使能，NVIC使能。

   - BLE模式：将消息队列同步给upperstack。NVIC会等upperstack初始化完成再使能：在 :func:`app_handle_dev_state_evt` 函数中 **GAP_INIT_STATE_STACK_READY** 状态下使能NVIC。

   - USB模式：USB使能，NVIC使能。

.. figure:: ../figures/Initializations_in_app_main_task.*
   :align: center
   :scale: 80%
   :name: app_main_task函数初始化内容

   app_main_task函数初始化内容

消息和事件处理流程
---------------------

.. figure:: ../figures/mouse_message_handling_flow.*
   :align: center
   :name: Tri-Mode Mouse Message Handling Flow CN

   Tri-Mode Mouse Message Handling Flow


.. list-table:: Tri-Mode Mouse软件模块说明表
   :header-rows: 1

   * - 模块
     - 说明

   * - Qdecoder module
     - 滚轮模块

   * - Led module
     - LED灯效模块

   * - Mode monitor module
     - 模式切换模块，识别并及时切换鼠标的三种模式
      
       (BLE/2.4G/USB mode)

   * - Button module
     - 按键模块，支持GPIO按键和Keyscan按键

   * - Sensor module
     - 光学传感器模块，本文以PAW3395为例

   * - Battery module
     - 电池电量模块，包括电量的定时检测，低电量的处理等

   * - USB module
     - USB模块，支持Full/High Speed USB

   * - Watch dog module
     - 看门狗模块，包括CPU active和DLPS状态下两种看门狗

上图为Tri-Mode Mouse应用的消息和事件处理流程图，SDK中会通过软件抽象层获取或设置各个外设模块的状态、行为和数据。需要及时处理的行为或数据，会直接在外设的中断处理函数中进行处理；对实时性要求不高的行为或数据，会发送消息给app task，等app task得到调度后在消息处理函数中做处理。以按键模块为例，在对应的中断处理函数中进行按键数据的处理和发送，而组合键等的识别和处理，会通过消息机制发送给app task处理。

GAP层通过MSG和Event机制通知APP层，APP层通过API调用GAP层函数。:cpp:func:`gap_handle_msg` 中有详细的GAP消息/事件描述。

状态机
---------

BLE/2.4G/USB三种传输模式切换
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

模式拨片位置的确定
^^^^^^^^^^^^^^^^^^^^

模式切换的拨片能拨到三个档位，OFF档、BLE mode档和2.4G mode档。相关管脚在 :file:`board.h` 中定义：

.. code-block:: c

   #define MODE_MONITOR_EN

   #if MODE_MONITOR_EN
   #define BLE_MODE_MONITOR                  XI32K
   #define BLE_MODE_MONITOR_IRQ              GPIOA17_IRQn
   #define ble_mode_monitor_int_handler      GPIOA17_Handler

   #define PPT_MODE_MONITOR                  XO32K
   #define PPT_MODE_MONITOR_IRQ              GPIOA18_IRQn
   #define ppt_mode_monitor_int_handler      GPIOA18_Handler

   #define USB_MODE_MONITOR                  P1_2
   #define USB_MODE_MONITOR_IRQ              GPIOA10_IRQn
   #define usb_mode_monitor_int_handler      GPIOA10_Handler
   #endif

根据 BLE_MODE_MONITOR 和 PPT_MODE_MONITOR 的电平高低情况判断当前拨片位置：

   - BLE_MODE_MONITOR 电平为低，PPT_MODE_MONITOR为高，拨片位置在BLE mode档。

   - BLE_MODE_MONITOR 电平为高，PPT_MODE_MONITOR为低，拨片位置在2.4G mode档。

   - BLE_MODE_MONITOR 电平为高，PPT_MODE_MONITOR为高，拨片位置在中间OFF档。

根据 USB_MODE_MONITOR 的电平高低判断当前USB是否插入，USB_MODE_MONITOR 为高电平时表示USB插入。其相关的判断和处理在 :file:`mode_monitor_driver.c` 和 :file:`mode_monitor_handle.c` 中。

仅根据拨片位置选择模式
^^^^^^^^^^^^^^^^^^^^^^^

:file:`board.h` 中宏定义 **FEATURE_ALWAYS_IN_USB_MODE_WHTH_USB_INSET** 设置为0，鼠标的模式完全根据模式切换的拨片位置来决定：

   - 拨片位置在BLE mode档：鼠标处于BLE模式，USB插入不会切换模式，仅进行充电。

   - 拨片位置在2.4G mode档：鼠标处于2.4G模式，USB插入不会切换模式，仅进行充电。

   - 拨片位置在off档，且USB插入，鼠标处于USB模式。

插入USB即为USB模式
^^^^^^^^^^^^^^^^^^^^

:file:`board.h` 中宏定义 **FEATURE_ALWAYS_IN_USB_MODE_WHTH_USB_INSET** 设置为1，模式切换规则如下：

   - 当USB没有被插入时，鼠标模式由拨片的位置决定。

   - 当USB插入后，且USB枚举成功，不管鼠标处于什么模式都会重启并进入USB模式。

   - 当USB插入后，但USB枚举失败，鼠标仍然会处于当前模式不变化，仅仅进行充电。

BLE状态机
~~~~~~~~~~~~

.. figure:: ../figures/BLE_mode_state_switching_condition.*
   :align: center
   :scale: 70%
   :name: BLE模式状态转换

   BLE模式状态转换

.. list-table:: BLE模式状态转换条件
   :widths: 5 60
   :header-rows: 1

   * - 序号
     - 说明

   * - 1
     - Power On after GAP ready

   * - 2
     - When APP call le_adv_start in idle status

   * - 3
     - High duty cycle direct advertising time out, no connect request received

   * - 4
     - When APP call le_adv_stop in advertising status

   * - 5
     - When BT stack send GAP state change callback message from advertising to idle status

   * - 6
     - When connection established

   * - 7
     - When connection terminates in connected status

   * - 8
     - When pairing successfully in connected status

   * - 9
     - When connection terminates in paired status

   * - 10
     - When BT stack sends GAP state change callback message from connection to idle status

   * - 11
     - When low power voltage is detected in idle status

   * - 12
     - When normal power voltage is detected in low power status

2.4G状态机
~~~~~~~~~~~~~

.. figure:: ../figures/PPT_mode_state_switching_condition.*
   :align: center
   :scale: 70%
   :name: 2.4G模式状态转换

   2.4G模式状态转换

.. list-table:: 2.4G模式状态转换条件
   :widths: 5 60
   :header-rows: 1

   * - 序号
     - 说明

   * - 1
     - Power On after 2.4G driver init

   * - 2
     - When APP calls ppt_pair in idle status

   * - 3
     - When pairing time out, SYNC_EVENT_PAIR_TIMEOUT event received

   * - 4
     - When pairing successfully, SYNC_EVENT_PAIRED event received

   * - 5
     - When 2.4G link is lost in paired status, SYNC_EVENT_CONNECT_LOST event received

   * - 6
     - When APP calls ppt_reconnect in idle status

   * - 7
     - When connecting time out, SYNC_EVENT_PAIR_TIMEOUT event received

   * - 8
     - When connecting successfully in paired status, MOUSE_PPT_STATUS_CONNECTED event received

   * - 9
     - When connecting successfully, MOUSE_PPT_STATUS_CONNECTED event received

   * - 10
     - When 2.4G link lost in connected status, SYNC_EVENT_CONNECT_LOST event received

   * - 11
     - When low power voltage detected in idle status

   * - 12
     - When normal power voltage detected in low power status

定时器
---------

软件定时器
~~~~~~~~~~~~~

App默认能使用的软件定时器个数为32个，可以在 :file:`otp_config.h` 中添加宏 **TIMER_MAX_NUMBER** 修改软件定时器个数。目前鼠标用到的软件定时器如下表所示。

.. list-table:: 软件定时器说明
   :widths: 5 20 30
   :header-rows: 1

   * - 序号
     - 软件定时器
     - 说明

   * - 1
     - adv_timer
     - 超时停止广播

   * - 2
     - update_conn_params_timer
     - 连接后进行连接参数更新操作

   * - 3
     - next_state_check_timer
     - 配对连接时BLE状态检测

   * - 4
     - achieve_ble_servie_timer
     - BLE配对时确保获取服务后再使能sensor

   * - 5
     - no_act_disconn_timer
     - 长时间无操作，鼠标主动断开BLE连接

   * - 6
     - watch_dog_reset_dlps_timer
     - Watch Dog定时喂狗

   * - 7
     - ble_mode_monitor_debounce_timer
     - BLE_MODE_MONITOR pin脚gpio电平检测去抖

   * - 8
     - ppt_mode_monitor_debounce_timer
     - PPT_MODE_MONITOR pin脚gpio电平检测去抖

   * - 9
     - usb_mode_monitor_debounce_timer
     - USB_MODE_MONITOR pin脚gpio电平检测去抖

   * - 10
     - qdec_allow_enter_dlps_timer
     - 避免滚轮模块引起的长时间无法进入DLPS

   * - 11
     - combine_keys_detection_timer
     - 检测组合按键状态

   * - 12
     - long_press_key_detect_timer
     - 长按键的检测

   * - 13
     - keys_press_check_timer
     - 避免按键模块引起的长时间无法进入DLPS

   * - 14
     - remote_wake_up_flag_timer
     - 避免USB重复多次执行remote wakeup

   * - 15
     - led_gpio_ctrl_timer
     - 通过PAD方式驱动LED的控制

   * - 16
     - bat_detect_timer
     - 电池电量定时检测

   * - 17
     - cfu_status_check_timer
     - 通过USB升级固件时的状态检查

   * - 18
     - single_tone_timer
     - 进入产测模式后启动USB模块

   * - 19
     - single_tone_exit_timer
     - 产测模式下通过HCI指令控制时使用

.. _tri-mode mouse硬件定时器:

硬件定时器
~~~~~~~~~~~~~

有两种硬件定时器可以供app使用，8个普通的HW Timer和4个Enhance Timer，具体的特性、区别和使用方式参考datasheet，目前鼠标工程中已经使用的定时器有如下表所示。

.. list-table:: 硬件定时器说明
   :widths: 5 10 30
   :header-rows: 1

   * - 序号
     - 硬件定时器
     - 说明

   * - 1
     - TIM0
     - 蓝牙协议栈已使用，app无法使用

   * - 2
     - TIM1
     - 蓝牙协议栈已使用，app无法使用

   * - 3
     - TIM2
     - app LED模块用来输出PWM波控制RGB LED

   * - 4
     - TIM5
     - app用来定时读取光学传感器的x,y数据

   * - 5
     - TIM6
     - app LED模块使用检查和控制RGB LED的状态和颜色变化

   * - 6
     - ENH_TIM0
     - 2.4G协议栈已使用，app无法使用

   * - 7
     - ENH_TIM1
     - 2.4G协议栈已使用，app无法使用

   * - 8
     - ENH_TIM2
     - dongle端已被2.4G协议栈使用，无法被app使用；
     
       mouse端未被2.4G协议栈使用，app LED模块用来输出PWM波控制RGB LED

   * - 9
     - ENH_TIM3
     - app LED模块用来输出PWM波控制RGB LED

BLE模式
----------

BLE初始化
~~~~~~~~~~~

鼠标处于BLE蓝牙模式，上电是需要对蓝牙相关内容进行初始化，包括如下：

1. :func:`main` 函数中：

   其中蓝牙的地址，设备名称，默认的广播参数，绑定相关参数等都在 :func:`app_le_gap_init` 中；服务的注册在 :func:`app_le_profile_init` 中。

.. code-block:: c

   le_gap_init(1);
   gap_lib_init();
   app_le_gap_init();
   app_le_profile_init();

2. :func:`app_main_task` 中：

.. code-block:: c

   gap_start_bt_stack(evt_queue_handle, io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

3. :func:`app_handle_dev_state_evt` 中：

   当BLE协议栈初始化完成后，会通过消息机制通知app，调用 :func:`app_handle_dev_state_evt` 接口，进行配对信息的获取，回连广播的发送，以及NVIC的使能。

.. code-block:: c

   if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
   {
      APP_PRINT_INFO0("GAP stack ready");
      ......
   }

HID服务
~~~~~~~~~~

BLE模式下主要的服务为HID service，其中HID描述符在 :file:`hids_ms.c` 中由数组 :mod:`hids_report_descriptor` 定义。

BLE广播
~~~~~~~~~~

广播类型
^^^^^^^^^^^

鼠标使用的广播包均为非定向广播：Undirected advertising event。其中，AdvA字段是发advertising封包设备的地址，AdvData格式如下图所示。

.. figure:: ../figures/ADV_IND_PDU_Payload.*
   :align: center
   :scale: 70%
   :name: ADV_IND PDU Payload CN

   ADV_IND PDU Payload

.. figure:: ../figures/Advertising_and_Scan_Response_data_format.*
   :align: center
   :scale: 60%
   :name: Advertising and Scan Response data format CN

   Advertising and Scan Response data format

鼠标应用程序通过调用 :func:`mouse_start_adv` 进行发送广播，并设定广播类型，鼠标广播类型枚举如下。

.. code-block:: c

   typedef enum
   {
      ADV_IDLE = 0,
      ADV_DIRECT_HDC,
      ADV_UNDIRECT_RECONNECT,
      ADV_UNDIRECT_PAIRING,
   } T_ADV_TYPE;

鼠标用到其中两种广播类型：ADV_UNDIRECT_PAIRING，ADV_UNDIRECT_RECONNECT。分别对应配对广播和回连广播。不建议使用ADV_DIRECT_HDC回连，有些电脑或平板不支持定向广播回连。

配对广播
^^^^^^^^^^^

鼠标在配对模式时发送配对广播包，用于和对端设备配对连接。配对广播包的格式为Undirected Advertising Packet，Advertising Interval范围建议设为0x20 - 0x30（即20ms - 30ms），广播超时时间通过宏 **ADV_UNDIRECT_PAIRING_TIMEOUT** 进行设置，默认为60秒。广播内容具体如下：

.. list-table:: 配对广播包内容
   :header-rows: 1

   * - Flag Field (3 bytes)
     - Appearance Field – Device type (4 bytes)
     - Service Field (4 bytes)
     - Local Name Field (<= 20 bytes)

   * - 0x02, 0x01, 0x05
     - 0x03, 0x19, 0xc2, 0x03
     - 0x03, 0x03, 0x12, 0x18
     - C_DEVICE_NAME_LEN,

       0x09,

       C_DEVICE_NAME

其中 C_DEVICE_NAME_LEN 和 C_DEVICE_NAME 默认如下：

.. code-block:: c

   #define C_DEVICE_NAME  'B', 'L', 'E','_', 'M', 'O', 'U', 'S', 'E', '(', '0', '0', ':', '0', '0', ')'
   #define C_DEVICE_NAME_LEN    (16+1)  /* sizeof(C_DEVICE_NAME) + 1 */

回连广播
^^^^^^^^^^^

鼠标应用配置宏定义 **FEATURE_SUPPORT_PRIVACY** 必须配置为1，即打开随机地址解析功能。鼠标采用 Undirected Advertising + White List 方式进行回连，Advertising Interval范围建议设为0x20 - 0x30（即20ms - 30ms），广播超时时间通过宏 **ADV_UNDIRECT_RECONNECT_TIMEOUT** 进行设置，默认为60秒。广播具体内容如下，其中Flags建议设置为 GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED(0x04)，这样只有曾经与之配对过的设备扫描到该广播会显示在设备列表里面。

.. list-table:: 回连广播包内容
   :header-rows: 1

   * - Flag Field (3 bytes)
     - Appearance Field – Device type (4 bytes)
     - Service Field (4 bytes)
     - Local Name Field (<= 20 bytes)

   * - 0x02, 0x01, 0x04
     - 0x03, 0x19, 0xc2, 0x03
     - 0x03, 0x03, 0x12, 0x18
     - C_DEVICE_NAME_LEN,

       0x09,

       C_DEVICE_NAME

广播发送和停止
^^^^^^^^^^^^^^^^

鼠标应用程序通过调用 :func:`mouse_start_adv` 进行发送广播。鼠标发送的配对和回连广播，都是通过调用 :func:`mouse_stop_adv` 来停止广播。停止广播的原因如下：

.. code-block:: c

   typedef enum
   {
      STOP_ADV_REASON_IDLE = 0,
      STOP_ADV_REASON_PAIRING,
      STOP_ADV_REASON_TIMEOUT,
      STOP_ADV_REASON_LOWPOWER,
   } T_STOP_ADV_REASON;

.. list-table:: BLE停止广播原因
   :widths: 5 20 20
   :header-rows: 1


   * - 序号
     - 原因
     - 说明

   * - 1
     - STOP_ADV_REASON_IDLE
     - 收到ADV_DIRECT_HDC广播停止的stack callback message

   * - 2
     - STOP_ADV_REASON_PAIRING
     - 要进行配对广播的发送，停止当前的广播

   * - 3
     - STOP_ADV_REASON_TIMEOUT
     - APP广播超时后调用le_adv_stop停止广播

   * - 4
     - STOP_ADV_REASON_LOWPOWER
     - 停止广播，进入Low Power模式


停止广播后，不同原因可能会有不同的处理，均在 :func:`app_stop_adv_reason_handler` 中实现。

BLE配对和连接
~~~~~~~~~~~~~~~

BLE配对
^^^^^^^^^^

鼠标触发配对有以下几种情况：

   1. 鼠标没有配对信息时：

      1. 长按组合键 :kbd:`左+中+右` 3秒，可以进入配对模式，发配对广播和设备进行配对连接。
      2. 当宏定义 **FEATURE_SUPPORT_AUTO_PAIR_WHEN_POWER_ON** 设置为1时，鼠标上电即可触发配对。

   2. | 鼠标已经配对过，有保存配对信息：
      | 长按组合键 :kbd:`左+中+右` 3秒，鼠标会生成新的static random address，进入配对模式，发配对广播。鼠标和某设备配对上后，设备不解除配对的情况下，鼠标可以重新与该设备配对。

BLE回连
^^^^^^^^^^

鼠标通过发送回连广播包，用于鼠标保存有配对信息时迅速和对端设备建立连接。

在以下三种情况下，鼠标会发送回连广播包进行回连：

   1. 鼠标上电，如果鼠标成功配对过并且保存配对信息，在上电初始化完成后发送回连广播尝试回连。
   2. 鼠标和对端配对连接上后，发生了非预期断线（不是任何一方主动断线），鼠标会发送回连广播尝试回连。
   3. 鼠标和对端配对连接上后，有一方主动断线，重新使用鼠标时（移动，按键，滚轮），会发送回连广播尝试回连。

VID和PID
^^^^^^^^^^^

BLE的默认 :term:`VID` 和 :term:`PID` 在 :file:`board.h` 中，如下：

.. code-block:: c

   #define C_VID        0x005D
   #define C_PID        0x0426

连接参数
^^^^^^^^^^

默认的连接参数在 :file:`mouse_applicaiton.h` 中，如下：

.. code-block:: c

   #define MOUSE_CONNECT_INTERVAL         0x06  /*0x06 * 1.25ms = 7.5ms*/
   #define MOUSE_CONNECT_LATENCY          99
   #define MOUSE_SUPERVISION_TIMEOUT      4500  /* 4.5s */

BLE断线
^^^^^^^^^^

鼠标和对端建立连线后，在以下三种情况下会断线：

   1. 链路异常而导致的断线（如超出连线距离，对端设备断电等），鼠标会发送回连广播，尝试回连。
   2. 对端主动和鼠标进行断线。
   3. 鼠标应用层调用 :func:`mouse_terminate_connection` 来主动和对端进行断线。

其中鼠标主动断线的原因包括如下：

.. code-block:: c

   typedef enum
   {
      DISCONN_REASON_IDLE = 0,
      DISCONN_REASON_PAIRING,
      DISCONN_REASON_TIMEOUT,
      DISCONN_REASON_PAIR_FAILED,
      DISCONN_REASON_LOW_POWER,
      DISCONN_REASON_ADDRESS_SWITCH,
      DISCONN_REASON_MOUSE_MODE_SWITCH_TO_USB,
   } T_DISCONN_REASON;

.. list-table:: BLE主动断线原因
   :widths: 5 20 20
   :header-rows: 1

   * - 序号
     - 原因
     - 说明

   * - 1
     - DISCONN_REASON_IDLE
     - 初始化的默认值

   * - 2
     - DISCONN_REASON_PAIRING
     - 要进行配对广播的发送，断开BLE连接

   * - 3
     - DISCONN_REASON_TIMEOUT
     - 当宏FEATURE_SUPPORT_NO_ACTION_DISCONN设置为1时，无操作时间达到设定的timeout时间

   * - 4
     - DISCONN_REASON_PAIR_FAILED
     - 配对失败，断开当前的连接

   * - 5
     - DISCONN_REASON_LOW_POWER
     - 要进入Low Power Status，断开BLE连接

   * - 6
     - DISCONN_REASON_ADDRESS_SWITCH
     - 当打开多地址切换功能时，切换当前ble的static random address

   * - 7
     - DISCONN_REASON_MOUSE_MODE_SWITCH_TO_USB
     - 当宏FEATURE_ALWAYS_IN_USB_MODE_WHTH_USB_INSET设置为1时，在非USB模式下，插入USB并枚举成功

主动断线后，不同原因会有不同的处理，均在 :func:`app_disconn_reason_handler` 中实现。

BLE数据发送
~~~~~~~~~~~~~

鼠标数据通过接口 :mod:`app_ble_send_mouse_data(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index, T_MOUSE_DATA *mouse_data, uint16_t data_len, T_GATT_PDU_TYPE type)` 进行发送。其中结构体 :mod:`T_MOUSE_DATA` 定义如下：

.. code-block:: c

   typedef struct t_mouse_data
   {
      uint8_t button;
      uint16_t x;
      uint16_t y;
      uint8_t v_wheel;
      uint8_t h_wheel;
   } T_MOUSE_DATA;

其他数据，如Keyboard，Consumer或者Vendor数据，通过接口 :mod:`app_ble_send_data(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index, uint8_t *p_data, uint16_t data_len, T_GATT_PDU_TYPE type)` 来发送。

其他相关功能
~~~~~~~~~~~~~~

Privacy解析
^^^^^^^^^^^^^^

只有将 :file:`board.h` 中的宏定义 **FEATURE_SUPPORT_PRIVACY** 设置为1，才能打开privacy解析功能，能够解析random address。鼠标应用必须要将该宏定义置1，以便可以和random address的设备进行配对和回连。

iOS配对
^^^^^^^^^^

由于和 :term:`iOS` 配对有安全性的要求，鼠标必须将 :file:`board.h` 中的宏定义 **FEATURE_SUPPORT_HIDS_CHAR_AUTHEN_REQ** 置1以便可以和 :term:`iOS` 系统的设备进行配对和回连。

配对信息的保存和恢复
^^^^^^^^^^^^^^^^^^^^^

当 :file:`board.h` 中的宏定义 **FEATURE_SUPPORT_REMOVE_LINK_KEY_BEFORE_PAIRING** 置1时，鼠标在发配对广播进行配对前，首先会清除原本的配对信息。

如果宏定义 **FEATURE_SUPPORT_REMOVE_LINK_KEY_BEFORE_PAIRING** 和 **FEATURE_SUPPORT_RECOVER_PAIR_INFO** 同时置为1，鼠标会在清除配对信息前先进行备份（包括鼠标自己的Static address），以便本次配对失败（包括但不限于配对超时、配对失败和中途下电等等）后，可以恢复原有的配对信息，和原来的设备进行回连；如果配对成功，原本配对信息的备份会被清除。

建议将 **FEATURE_SUPPORT_REMOVE_LINK_KEY_BEFORE_PAIRING** 和 **FEATURE_SUPPORT_RECOVER_PAIR_INFO** 均置为1。

Data Length Extension
^^^^^^^^^^^^^^^^^^^^^^^^^

当 :file:`board.h` 中的宏定义 **FEATURE_SUPPORT_DATA_LENGTH_EXTENSION** 置1时，鼠标和对端设备连接上后会主动请求将链路层的data length更新为251。如果将该宏置1，可以提高长包的交互速度。默认置0。

不检查CCCD
^^^^^^^^^^^^^

当 :file:`board.h` 中的宏定义 **FEATURE_SUPPORT_NO_CHECK_CCCD** 置1时，鼠标和对端设备连接上，不需要对端更新client characteristic configuration，鼠标就可以发送notification或indication。

鼠标地址选择
^^^^^^^^^^^^^^

可以配置 :file:`board.h` 中的宏定义 **FEATURE_MAC_ADDR_TYPE** 来选择鼠标所使用的蓝牙地址类型，包括了：public address，单一的static address，可切换的多个static address。

默认使用单一的static address。

.. code-block:: c

   #define FEATURE_SUPPORT_PUBLIC_ADDR                      0  **../* use public addr*/
   #define FEATURE_SUPPORT_SINGLE_LOCAL_STATIC_ADDR         1  **../* use single local ramdon addr*/
   #define FEATURE_SUPPORT_MULTIPLE_LOCAL_STATIC_ADDR       2  **../* use multiple local ramdon addr \*/
   #define FEATURE_MAC_ADDR_TYPE                            FEATURE_SUPPORT_SINGLE_LOCAL_STATIC_ADDR
   #if (FEATURE_MAC_ADDR_TYPE == FEATURE_SUPPORT_MULTIPLE_LOCAL_STATIC_ADDR)
   #define APP_MAX_BOND_NUM         2
   #endif

Public地址
*************

当 :file:`board.h` 中的宏定义 **FEATURE_MAC_ADDR_TYPE** 配置为 **FEATURE_SUPPORT_PUBLIC_ADDR** 时，鼠标使用public address，即为config file中配置的MAC address。当使用该地址时，鼠标在配对时地址不会改变。当与对端设备配对成功后，如果想要与该设备重新配对，则需要先从对端设备的设备列表中解除配对，才能重新配对。不推荐鼠标使用public address。

单一Static地址
*****************

当 :file:`board.h` 中的宏定义 **FEATURE_MAC_ADDR_TYPE** 配置为 **FEATURE_SUPPORT_SINGLE_LOCAL_STATIC_ADDR** 时，鼠标使用单一的static address。

上电时鼠标会根据MAC address随机生成一个st­a­tic address作为自己的地址来和对端设备进行配对和回连等，在重新配对前不会改变当前的static address。当鼠标已经生成了static address，并且和某一个设备配对过，本地有配对信息时，鼠标重新发起配对，会重新随机生成一个static address作为新的地址。

多Static地址切换
*******************

当鼠标需要和多个设备进行配对连接，在多个设备间快速切换时，需要把 :file:`board.h` 中的宏定义 **FEATURE_MAC_ADDR_TYPE** 配置为 **FEATURE_SUPPORT_MULTIPLE_LOCAL_STATIC_ADDR**，鼠标使用可切换的多static address。可切换的地址数量可以通过宏定义 **APP_MAX_BOND_NUM** 来修改，默认数量为2个。

上电时鼠标会根据MAC address随机生成多个static address作为自己的地址来和对端设备进行配对和回连等。这些static address单独来看和单一的static address在使用上是完全一样的，每个static address都可以单独的和一个设备配对连接。多个static address可以进行切换，切换static address就相当于在已配对的多个设备间进行切换。默认是通过组合键 :kbd:`滚轮中键+前进键` 来进行多地址的切换。

Tx Power设置
^^^^^^^^^^^^^^^

当 :file:`board.h` 中宏 **FEATURE_SUPPORT_APP_CFG_BLE_TX_POWER** 设置为1时（默认为0），可以单独设置BLE模式的tx power，否则tx power由config file中的配置决定。

2.4G模式
-----------

2.4G初始化
~~~~~~~~~~~~~

鼠标处于2.4G模式，上电是需要对2.4G相关内容进行初始化，包括如下：

:func:`app_main_task` 函数中：

   .. code-block:: c

      if (app_global_data.mode_type == PPT_2_4G)
      {
         os_delay(100);
         mouse_ppt_init();
         pwr_mgr_init();
         mouse_ppt_enable();
         app_nvic_config();
      }

其中 :mod:`os_delay(100)` 是系统延时100ms，保证2.4G RF相关的上电初始化完成，可以进行后续2.4G的初始化；:func:`mouse_ppt_init` 是对2.4G相关的初始化；:func:`pwr_mgr_init` 是电源模式初始化，必须在2.4G初始化之后；:func:`mouse_ppt_enable` 是使能2.4G模块。

其中 :func:`mouse_ppt_init` 中主要包括了：

   1. | 配置2.4­­­G角色。
      | 鼠标：master，接收器：slave。

   2. 添加一些callback函数，包括接收到数据后调用的callbak函数 :mod:`ppt_app_receive_msg_cb`；发送完数据后调用的callback函数 :mod:`ppt_app_send_msg_cb`；2.4G事件处理的callback函数 :func:`ppt_app_sync_event_cb`，包括的事件有：

      1. SYNC_EVENT_PAIRED：2.4G配对成功，该事件产生后，会马上产生 SYNC_EVENT_CONNECTED 事件。
      2. SYNC_EVENT_PAIR_TIMEOUT：2.4G配对超时。
      3. SYNC_EVENT_CONNECTED：2.4G成功建立连接，回连成功会产生该事件，配对成功会先产生 SYNC_EVENT_PAIRED 再产生该事件。
      4. SYNC_EVENT_CONNECT_TIMEOUT：2.4G回连超时。
      5. SYNC_EVENT_CONNECT_LOST：链路异常断线。

   3. 获取绑定信息。

      .. code-block:: c

         ppt_app_global_data.is_ppt_bond = ppt_check_is_bonded()

   4. | 设置配对时的rssi限制。
      | :mod:`sync_pair_rssi_set(-65)`：表示rssi必须大于-65dbm才允许配对，2.4g master和slave两端都可以各自单独配置。

   5. 设置2.4G的连接参数。

      1. 设置数据包通信间隔和重传间隔：:func:`mouse_ppt_set_sync_interval`。
      2. 设置心跳包间隔：:mod:`sync_master_set_hb_param(2, PPT_DEFAULT_HEARTBEAT_INTERVAL_TIME, 0)`。
      3. 设置CRC校验参数，默认校验长度为8 bit：:func:`sync_crc_set(8, 0x07, 0xff)`。

   6. 设置不同2.4G传输类型数据的缓存buffer深度：2.4g driver可以缓存一些发送的数据，不同的数据类型有各自的buffer。

      .. code-block:: c

         /**
          *  Different message types have different queue size, from left to right correspond to SYNC_MSG_TYPE_ONESHOT,
          *  SYNC_MSG_TYPE_FINITE_RETRANS, SYNC_MSG_TYPE_INFINITE_RETRANS, and SYNC_MSG_TYPE_DYNAMIC_RETRANS, respectively.
          */
         uint8_t msg_quota[SYNC_MSG_TYPE_NUM] = {0, 2, 2, 2};
         sync_msg_set_quota(msg_quota);

      :mod:`{0, 2, 2, 2}` 表示 **SYNC_MSG_TYPE_ONESHOT**, **SYNC_MSG_TYPE_FINITE_RETRANS**, **SYNC_MSG_TYPE_INFINITE_RETRANS**, **SYNC_MSG_TYPE_DYNAMIC_RETRANS** 四种数据类型的缓存buffer深度分别设置为0, 2, 2, 2。

   7. | 设置2.4G tx power。
      | 当宏 **FEATURE_SUPPORT_APP_CFG_PPT_TX_POWER** 置1时，可以通过 :mod:`sync_tx_power_set(false, PPT_TX_POWER_DBM_MAX, PPT_TX_POWER_DBM_MIN)` 设置2.4G tx power。否则tx power由config file中的配置决定。

2.4G配对和连接
~~~~~~~~~~~~~~~~

2.4G配对
^^^^^^^^^^^^

鼠标程序中调用 :func:`mouse_ppt_pair` 发起配对，持续1秒。如果配对成功，会依次产生 SYNC_EVENT_PAIRED 和 SYNC_EVENT_CONNECTED 两个事件通知app并进行相应的处理。如果1秒内没有配对成功，会产生 SYNC_EVENT_PAIR_TIMEOUT 事件，会重新尝试配对，尝试的次数可以通过宏定义 **PPT_PAIR_TIME_MAX_COUNT** 修改，默认为30次，即配对时长为30秒。

以下几种情况会触发配对：

   1. 长按组合键 :kbd:`左+中+右` 3秒，触发配对。
   2. 当宏定义 **FEATURE_SUPPORT_AUTO_PAIR_WHEN_POWER_ON** 设置为1时，鼠标上电后如果没有配对信息，即可触发配对。

2.4G回连
^^^^^^^^^^^^

鼠标程序中调用 :func:`mouse_ppt_reconnect` 发起回连，持续1秒。如果回连成功，会产生 SYNC_EVENT_CONNECTED 事件。如果1秒内没有回连成功，会产生 SYNC_EVENT_CONNECT_TIMEOUT 事件，会重新尝试回连，尝试的次数可以通过宏定义 **PPT_RECONNECT_TIME_MAX_COUNT** 修改，默认为4次，也就是4秒。

以下几种情况会触发回连：

   1. 上电后，2.4G有配对信息，会尝试回连。
   2. 当2.4G链路异常断线产生了 SYNC_EVENT_CONNECT_LOST 事件，会尝试回连。

发包间隔和上报率
^^^^^^^^^^^^^^^^^^^

鼠标（2.4G master）上电时在 :func:`mouse_ppt_init` 通过调用 :func:`mouse_ppt_set_sync_interval` 来设置2.4G正常数据通信时的发包间隔，发包间隔根据当前设置的上报率来配置，比如上报率为1KHz，发包间隔就设置为1000us。

接收器（2.4G slave）不需要设置发包间隔，接收器和鼠标配对上后，会根据鼠标的参数来调整。

如果在2.4G使能后要调整发包间隔，必须保证鼠标在2.4G idle状态，即需要断线，且不进行配对或回连，然后重新设置发包间隔即可，接口为 :func:`mouse_ppt_set_sync_interval`。

心跳包
^^^^^^^^^^

在2.4G建立连接以后，且没有数据交互时，2.4G会定期交互心跳包，以维持连接。以最后一笔数据交互为起始点，经过一段时间（10ms）没有发生新的数据交互（非空包），会开始通过心跳包维持连接。

鼠标（2.4G master）上电时在 :func:`mouse_ppt_init` 通过调用 :func:`sync_master_set_hb_param` 来设置2.4G心跳包的发包间隔，默认值为250ms。接收器（2.4G slave）不需要设置心跳包间隔。

.. code-block:: c

   /* set 2.4G connection heart beat interval */
   sync_master_set_hb_param(2, PPT_DEFAULT_HEARTBEAT_INTERVAL_TIME,0);

CRC校验
^^^^^^^^^^^

鼠标（2.4G master）的校验长度默认使用8 bit，可以在 :func:`mouse_ppt_init` 通过调用 :func:`sync_crc_set` 来配置。如果要在2.4g使用过程中重新配置校验长度，必须先调用 :func:`mouse_ppt_stop_sync` 断开2.4g连接。校验长度推荐使用16 bit，在实际使用中会更安全，但是应用层最大可传输长度会减少1 Byte，并且画线功耗略有增加。

2.4G断线
~~~~~~~~~~~~

在2.4G连接建立后，发生以下三种情况会认为连接已经断开：

   1. 有数据持续交互时，持续一段时间（3*发包间隔）没有交互成功。
   2. 无数据交互时，通过心跳包维持连接，以心跳包时刻起一段时间（心跳包间隔+3*发包间隔）没有交互成功。
   3. board.h中宏定义 **FEATURE_SUPPORT_NO_ACTION_DISCONN** 设置为1时，打开无操作断线的功能，当鼠标处于连接状态时，且一段时间没有被使用后，会主动断线。可以通过滚轮，按键或者移动鼠标进行回连。无操作断线的时间可以通过 :file:`swtimer.h` 中的宏定义 **NO_ACTION_DISCON_TIMEOUT** 来修改，默认时间为1分钟。

除主动断线外，其他异常断线情况都会收到 SYNC_EVENT_CONNECT_LOST 事件，会尝试回连。

2.4G数据传输
~~~~~~~~~~~~~~~~

2.4G传输类型
^^^^^^^^^^^^^^^^^^

2.4G的传输类型有以下4种：

   1. SYNC_MSG_TYPE_ONESHOT：只发送一次，不重传。
   2. SYNC_MSG_TYPE_FINITE_RETRANS：有限次数的重传发送，通过 :func:`sync_msg_set_finite_retrans` 来配置重传次数。
   3. SYNC_MSG_TYPE_INFINITE_RETRANS：无限重传的发送。
   4. SYNC_MSG_TYPE_DYNAMIC_RETRANS：动态重传的发送，会一直重传，直到有新数据需要发送。

.. code-block:: c

   typedef enum
   {
      SYNC_MSG_TYPE_ONESHOT,
      SYNC_MSG_TYPE_FINITE_RETRANS,
      SYNC_MSG_TYPE_INFINITE_RETRANS,
      SYNC_MSG_TYPE_DYNAMIC_RETRANS,
      SYNC_MSG_TYPE_NUM,
      SYNC_MSG_TYPE_ALL = SYNC_MSG_TYPE_NUM
   } sync_msg_type_t;

应用层数据类型
^^^^^^^^^^^^^^^^

接收器在收到2.4G数据后，需要根据不同的应用数据类型（比如鼠标数据，按键数据等等），通过USB往不同的通道（可能endpoint，report id等等不同）发送。为了区分不同的应用数据类型，鼠标端将发送的数据的前一个或两个字节作为Header，以表征应用数据类型和数据内容。Header内容参考 :mod:`T_PPT_SYNC_APP_HEADER`。

应用层数据长度
^^^^^^^^^^^^^^^^

在一个2.4G 发包间隔中，鼠标和接收器可以同时给对方发送数据，鼠标发送给接收器称之为上行，接收器发送给鼠标称之为下行，2.4G在一个发包间隔中上行和下行的应用层数据长度总和如下：

   - 发包间隔250us（上报率4KHz）：18 bytes

   - 发包间隔500us（上报率2KHz）：70 bytes

   - 发包间隔1ms（上报率1KHz）：127 bytes

2.4G数据长度最大不能超过127 bytes。当2.4G需要发送的数据，超过一个发包间隔内发包长度时，会占用后续的发包时间直到把当前数据全部发送完成，相当于根据数据长度，动态调整了发包间隔。

应答和重传
^^^^^^^^^^^^

2.4G交互是需要应答的，如果一方没有收到ACK，则认为这笔包没有发送成功，根据不同的2.4G数据传输类型和重传配置进行重传：

   1. SYNC_MSG_TYPE_ONESHOT：发送失败不重传。
   2. SYNC_MSG_TYPE_FINITE_RETRANS：有限次数的重传发送，重传次数通过 :mod:`sync_msg_set_finite_retrans` 来配置。
   3. SYNC_MSG_TYPE_INFINITE_RETRANS：无限重传。
   4. SYNC_MSG_TYPE_DYNAMIC_RETRANS：动态重传，当没有新的数据送到2.4G driver tx buffer中，tx buffer中的数据会无限重传，当有新数据送到2.4G driver tx buffer中时，tx buffer原本待发送的数据如果已经发送超过一次（重传0次）会停止重传，移出tx buffer。也就是数据重传的次数范围为0到无限次。

鼠标（2.4G master）在 :func:`mouse_ppt_set_sync_interval` 通过调用 :func:`sync_time_set` 来设置2.4G数据包重传间隔，调用接口时第一项参数选择 SYNC_TIME_PARAM_CONNECT_INTERVAL_HIGH，第二项参数即可设置具体的重传时间，默认是250us。当2.4G发包间隔超过250us时，每个发包间隔内有多次重传的机会（发包间隔/250us -1），比如发包间隔为1ms，当数据发送失败时，有3次重传机会，每250us重传一次。

Tx Power设置
~~~~~~~~~~~~~~~

当 :file:`board.h` 中宏 **FEATURE_SUPPORT_APP_CFG_PPT_TX_POWER** 设置为1时（默认为0），可以单独设置2.4G模式的tx power，否则tx power由config file中的配置决定。

USB模式
----------

USB状态
~~~~~~~~~~

USB的所有状态如下：

.. code-block:: c

   typedef enum
   {
      USB_PDN = 0,
      USB_ATTACHED = 1,
      USB_POWERED = 2,
      USB_DEFAULT = 3,
      USB_ADDRESSED = 4,
      USB_CONFIGURED = 5,
      USB_SUSPENDED = 6,
   } T_USB_POWER_STATE;

说明­：

   1. USB_PDN：上电后的默认状态。
   2. USB_ATTACHED：使能了USB，但没有打开USB clock。
   3. USB_POWERED：使能并打开了USB clock。
   4. USB_DEFAULT：USB插入并复位后的默认状态。
   5. USB_ADDRESSED：USB设备已经被分配地址。
   6. USB_CONFIGURED：USB设备已被配置，进入这个状态，一般认为USB枚举成功了。
   7. USB_SUSPENDED：USB进入suspend状态。

初始化
~~~~~~~~~

调用 :func:`usb_driver_init` 对USB模块进行初始化，包括：设置USB中断优先级，注册回调函数，初始化USB设备和配置描述符，初始化USB接口和端点，初始化HID。初始化过程如下：

   1. 设置USB中断优先级：:mod:`usb_isr_set_priority(3)`，默认设置为2（低于2.4g中断优先级，2.4g中断优先级为1）。
   2. 注册回调函数：:mod:`usb_dm_cb_register(app_usb_state_change_cb)` 和 :mod:`usb_spd_cb_register(app_usb_speed_cb)`，注册USB状态改变和USB speed通知的回调函数。app通过 :mod:`app_usb_state_change_cb` 获知当前的USB状态，通过 :mod:`app_usb_speed_cb` 获知当前是full speed还是high speed。
   3. 初始化USB设备描述符和配置描述符：:mod:`usb_dm_core_init(config)` 和 :func:`usb_dev_cfg_init`，进行设备描述符和配置描述符相关的初始化。
   4. 初始化USB接口：默认初始化了三个接口，初始化函数为：

      1. :func:`usb_interface_mouse_init`
      2. :func:`usb_interface_keyboard_init`
      3. :func:`usb_interface_dfu_init`

   5. 初始化HID：:func:`usb_hid_driver_init`。

设备描述符初始化
^^^^^^^^^^^^^^^^^^

在 :func:`usb_dev_cfg_init` 中，通过调用函数 :func:`usb_dev_driver_dev_desc_register` 和 :func:`usb_dev_driver_string_desc_register` 进行设备描述符的初始化，设备描述符相关内容通过 :file:`usb_device.c` 的两个局部变量 usb_dev_desc 和 dev_strings 来修改。其中USB VID默认为0x0BDA，USB PID默认为0x4762。

.. code-block:: c

   #define USB_VID            0x0BDA
   #define USB_PID            0x4762
   #define USB_BCD_DEVICE     0x0426
   static T_USB_DEVICE_DESC usb_dev_desc =
   {
      .bLength             = sizeof (T_USB_DEVICE_DESC),
      .bDescriptorType     = USB_DESC_TYPE_DEVICE,
      .bcdUSB              = 0x0200,
      .bDeviceClass        = 0,
      .bDeviceSubClass     = 0,
      .bDeviceProtocol     = 0,
      .bMaxPacketSize0     = 64,
      .idVendor            = USB_VID,
      .idProduct           = USB_PID,
      .bcdDevice           = USB_BCD_DEVICE,
      .iManufacturer       = STRING_ID_MANUFACTURER,
      .iProduct            = STRING_ID_PRODUCT,
      .iSerialNumber       = STRING_ID_SERIALNUM,
      .bNumConfigurations  = 1,
   };
   static T_STRING dev_strings[] =
   {
      [0] =
      {
         .id = STRING_ID_MANUFACTURER,
         .s  = "RealTek",
      },
      [1] =
      {
         .id = STRING_ID_PRODUCT,
         .s  = "RTK Mouse",
      },
      [2] =
      {
         .id = STRING_ID_SERIALNUM,
         .s  = "0123456789A",
      },
      [3] =
      {
         .id = STRING_ID_UNDEFINED,
         .s  = NULL,
      },
   };

配置描述符初始化
^^^^^^^^^^^^^^^^^^

在 :func:`usb_dev_cfg_init` 中，通过调用函数 :func:`usb_dev_driver_string_desc_unregister` 进行配置描述符的初始化，配置描述符相关内容通过 :file:`usb_device.c` 的变量 usb_cfg_desc 来修改。

.. code-block:: c

   static T_USB_CONFIG_DESC usb_cfg_desc =
   {
      .bLength = sizeof (T_USB_CONFIG_DESC),
      .bDescriptorType = USB_DESC_TYPE_CONFIG,
      .wTotalLength = 0xFFFF,
      //wTotalLengthwill be recomputed in usb lib according total interface descriptors
      .bNumInterfaces = 3,
      //bNumInterfaces will be recomputed in usb lib according total interface num
      .bConfigurationValue = DEFAULT_CONFIGURATION_VALUE,
      .iConfiguration = STRING_ID_UNDEFINED,
      .bmAttributes = REMOTE_WAKE_UP_ENALBE | RESERVED_TO_1_ENABLE,
      //suport remote wake up
      .bMaxPower = 25011.
   };

USB接口初始化
^^^^^^^^^^^^^^^

SDK中默认初始化了三个HID接口，mouse interface, keyboard interface和dfu interface，分别用于鼠标数据，键盘数据（keyboard，consumer和自定义数据）和DFU数据的交互。下面以mouse interface为例进行说明。

通过 :func:`usb_interface_mouse_init` 进行mouse interface的初始化，包括USB接口描述符，USB端点描述符和HID描述符的初始化，以及set/get report和set/get protocol的回调函数的注册。

.. code-block:: c

   void usb_interface_mouse_init(void)
   {
      inst = usb_hid_driver_inst_alloc();
   #if FEATURE_CHANGE_USB_INTERVAL_FOR_REPORT_RATE
      uint32_t usb_report_rate = get_report_rate_level_by_index(USB_MODE, app_global_data.usb_report_rate_index, app_global_data.max_report_rate_level);
      usb_set_mouse_interface_hs_interval(usb_report_rate);
   #endif
      usb_hid_driver_if_desc_register(inst, (void*)hid_if_descs_hs, (void*)hid_if_descs_fs, (void*)report_descs);
      T_USB_HID_DRIVER_CBS cbs;
      cbs.get_report = usb_hid_get_report;
      cbs.set_report = usb_hid_set_report;
      cbs.get_protocol = usb_hid_get_protocol;
      cbs.set_protocol = usb_hid_set_protocol;
      usb_hid_driver_cbs_register(inst, &cbs);
   }

* USB接口描述符为：

   .. code-block:: c

      static T_USB_INTERFACE_DESC hid_std_if_desc =
      {
         .bLength            = sizeof(T_USB_INTERFACE_DESC),
         .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
         .bInterfaceNumber   = USB_INTERFACE_NUM,
         .bAlternateSetting  = 0,
         .bNumEndpoints      = USB_EP_NUM,
         .bInterfaceClass    = USB_CLASS_CODE_HID,
         .bInterfaceSubClass = USB_SUBCLASS_HID_BOOT,
         .bInterfaceProtocol = HID_MOUSE_PROTOCOL,
         .iInterface         = 0,
      };

* USB端点描述符如下，其中只用到了一个端点：

   .. code-block:: c

      static T_USB_ENDPOINT_DESC int_in_ep_desc_fs =
      {
         .bLength           = sizeof(T_USB_ENDPOINT_DESC),
         .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
         .bEndpointAddress  = HID_INT_IN_EP_1,
         .bmAttributes      = USB_EP_TYPE_INT,
         .wMaxPacketSize    = 64,
         .bInterval         = 1,
      };

      static uint8_t hs_int_interval = 1;
      static T_USB_ENDPOINT_DESC int_in_ep_desc_hs =
      {
         .bLength           = sizeof(T_USB_ENDPOINT_DESC),
         .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
         .bEndpointAddress  = HID_INT_IN_EP_1,
         .bmAttributes      = USB_EP_TYPE_INT,
         .wMaxPacketSize    = 64,
         .bInterval         = 1,
      };

* HID描述符为：

   .. code-block:: c

      static T_HID_CS_IF_DESC  hid_cs_if_desc =
      {
         .bLength            = sizeof(T_HID_CS_IF_DESC),
         .bDescriptorType    = DESC_TYPE_HID,
         .bcdHID             = 0x0110,
         .bCountryCode       = 0,
         .bNumDescriptors    = 1,
         .desc[0]            =
         {
            .bDescriptorType = DESC_TYPE_REPORT,
            .wDescriptorLength = sizeof(report_descs),
         },
      };

USB Start/Stop
~~~~~~~~~~~~~~~~~

如果处于USB模式，上电时在 :mod:`app_main_task` 中调用 :func:`usb_start` 初始化和使能USB，并打开USB clock。

   .. code-block:: c

      void usb_start(void)
      {
         is_usb_allow_enter_dlps = false;

         usb_driver_init();

         APP_PRINT_INFO0("usb_start");
         usb_dm_start(false);
      }

当上电初始化完成后，程序运行过程中，可以通过 :func:`usb_stop` 完全关闭USB模块。

   .. code-block:: c

      void usb_stop(void)
      {
         APP_PRINT_INFO0("usb_stop");
         usb_state = USB_PDN;
         usb_dm_stop();

         is_usb_allow_enter_dlps = true;
      }

USB HID Class
~~~~~~~~~~~~~~~~

HID描述符和报告描述符
^^^^^^^^^^^^^^^^^^^^^^^

以mouse interface为例进行说明。

* ­HID描述符如下。

   .. code-block:: c

      static T_HID_CS_IF_DESC  hid_cs_if_desc =
      {
         .bLength            = sizeof(T_HID_CS_IF_DESC),
         .bDescriptorType    = DESC_TYPE_HID,
         .bcdHID             = 0x0110,
         .bCountryCode       = 0,
         .bNumDescriptors    = 1,
         .desc[0]            =
         {
            .bDescriptorType = DESC_TYPE_REPORT,
            .wDescriptorLength = sizeof(report_descs),
         },
      };

* 报告描述符可参考 :mod:`report_descs[]`。

Interrupt Report
^^^^^^^^^^^^^^^^^^^

以mouse interface为例进行说明。

通过 :func:`app_usb_send_mouse_data` 进行Interrupt report。:func:`app_usb_send_mouse_data` 调用了 :func:`usb_send_mouse_data` ，:func:`usb_send_mouse_data` 调用了 :func:`usb_send_data`。

:func:`usb_send_data` 有三个参数，USB report id，数据指针和数据长度。首次调用该函数会调用 :func:`usb_mouse_pipe_open` 初始化USB缓存队列，以存放interrupt report data。其中 :mod:`.high_throughput = 1` 表示interrupt report相关的处理都会直接在USB中断处理函数中进行，保证实时性；否则发送消息给usb task，在usb task中进行处理。MOUSE_MAX_TRANSMISSION_UNIT_SIZE为缓存队列中单元的最大byte size。MOUSE_MAX_PIPE_DATA_NUM为缓存队列的最大深度，当数据溢出时会丢弃最老的数据，保证新数据能正常入队。

.. code-block:: c

   /**
    * @brief  Open usb pipe
    * @param  None
    * @return None
    */
   static void *usb_mouse_pipe_open(void)
   {
      T_USB_HID_DRIVER_ATTR attr =
      {
         .zlp = 1,
         .high_throughput = 1,/*if it is set to 1, it can be executed in interrupt, else it execute in task.*/
         .congestion_ctrl = USB_PIPE_CONGESTION_CTRL_DROP_CUR,
         .rsv = 0,
         .mtu = MOUSE_MAX_TRANSMISSION_UNIT_SIZE
      };
      return usb_hid_driver_data_pipe_open(HID_INT_IN_EP_1, attr, MOUSE_MAX_PIPE_DATA_NUM, NULL);
   }

Boot Mode Report
^^^^^^^^^^^^^^^^^^^

以mouse interface为例进行说明。

在interface初始化函数中，注册set/get protocol的回调函数：

.. code-block:: c

   /**
    * @brief  USB interface init
    * @param  None
    * @return None
    */
   void usb_interface_mouse_init(void)
   {
      inst = usb_hid_driver_inst_alloc();
   #if FEATURE_CHANGE_USB_INTERVAL_FOR_REPORT_RATE
      uint32_t usb_report_rate = get_report_rate_level_by_index(USB_MODE,
                                                               app_global_data.usb_report_rate_index, app_global_data.max_report_rate_level);
      usb_set_mouse_interface_hs_interval(usb_report_rate);
   #endif
      usb_hid_driver_if_desc_register(inst, (void *)hid_if_descs_hs, (void *)hid_if_descs_fs,
                                       (void *)report_descs);

      T_USB_HID_DRIVER_CBS cbs = {0};
      cbs.get_report = usb_hid_get_report;
      cbs.set_report = usb_hid_set_report;
      cbs.get_protocol = usb_hid_get_protocol;
      cbs.set_protocol = usb_hid_set_protocol;
      usb_hid_driver_cbs_register(inst, &cbs);
   }

通过 :func:`usb_hid_set_protocol` 获取USB protocol，如果是HID_BOOT_PROTOCOL就意味着USB host在boot mode，此时为了兼容性考虑，修改上报率上限为1KHz，防止丢包。

如果USB protocol为HID_REPORT_PROTOCOL，:func:`usb_send_mouse_data` 和 :func:`usb_send_data` 中根据报告描述符发送数据(report id + report data)；如果为HID_BOOT_PROTOCOL，会按照boot mode所需的固定数据结构发送数据。

Set/Get Report
^^^^^^^^^^^^^^^^^

以dfu interface为例进行说明。

在interface初始化函数中，注册set/get report的回调函数：

.. code-block:: c

   void usb_interface_mouse_init(void)
   {
      ...
      cbs.get_report = usb_hid_get_report;
      cbs.set_report = usb_hid_set_report;
      usb_hid_driver_cbs_register(inst, &cbs);
   }

:func:`usb_hid_set_report` 有三个参数：usb report id，数据指针和数据长度指针。需要给数据指针和数据长度指针进行赋值。返回值为0时表示success。

.. code-block:: c

   static int usb_hid_get_report(uint8_t report_id, void *buf, uint16_t *len)
   {
      uint8_t *p_data = (uint8_t *)buf;
   #if FEATURE_SUPPORT_MP_TEST_MODE
   #if (THE_WAY_TO_ENTER_MP_TEST_MODE == ENTER_MP_TEST_MODE_BY_USB_CMD)
      if (report_id == REPORT_ID_MP_CMD)
      {
         p_data[0] = report_id;
         mp_test_get_report_handle(&p_data[1], len);
         *len += 1;
      }
      else
   #endif
   #endif
      {
   #if FEATURE_SUPPORT_USB_DFU
         p_data[0] = report_id;
         usb_dfu_handle_get_report_packet(report_id, &p_data[1], len);
         *len += 1;
   #endif
      }
      APP_PRINT_INFO2("[usb_hid_get_report] report_id = 0x%x, len = %d", report_id, *len);
      return 0;
   }

:func:`usb_hid_set_report` 有两个参数：数据指针和数据长度，其中数据的第一个字节为report id。返回值为0时表示success。

.. code-block:: c

   static int usb_hid_set_report(void *buf, uint16_t len)
   {
      uint8_t *p_data = (uint8_t *)buf;
      uint8_t report_id = p_data[0];
      APP_PRINT_INFO3("[usb_hid_set_report] report_id = 0x%x, len = %d, p_data = 0x %b", report_id,
                     len, TRACE_BINARY(len, p_data));
   #if FEATURE_SUPPORT_MP_TEST_MODE
   #if (THE_WAY_TO_ENTER_MP_TEST_MODE == ENTER_MP_TEST_MODE_BY_USB_CMD)
      if (report_id == REPORT_ID_MP_CMD)
      {
         mp_test_set_report_handle(&p_data[1], len - 1);
      }
      else
   #endif
   #endif
      {
   #if FEATURE_SUPPORT_USB_DFU
         usb_dfu_handle_set_report_packet(report_id, &p_data[1], len - 1);
   #endif
      }
      return 0;
   }

设备枚举
~~~~~~~~~~

USB设备进行枚举时，会进行 set/get descriptor 和 set/get config。当进行了set config后，USB状态会进行切换，在 :func:`app_usb_state_change_cb` 获知状态切换到了USB_CONFIGURED，认为USB设备枚举完成。USB模式下，状态切到USB_CONFIGURED后，鼠标数据才允许正常发送。宏 **FEATURE_ALWAYS_IN_USB_MODE_WHTH_USB_INSET** 设置为1时，插入USB即为USB模式，USB状态切到USB_CONFIGURED后，鼠标才会重启切到USB模式。

休眠和唤醒
~~~~~~~~~~~~~

USB suspend时候状态会切到USB_SUSPENDED，此时允许进入DLPS。

USB进入suspend以后，有两种方式唤醒：USB host wakeup和鼠标主动wakeup。使用鼠标尝试发送USB数据时，如果当前处于suspend状态，会主动进行wakeup。以鼠标数据发送为例：

.. code-block:: c

   bool app_usb_send_mouse_data(T_MOUSE_DATA *mouse_data)
   {
      ......
         if (usb_state == USB_SUSPENDED)
         {
               if (usb_wakeup_state == USB_WAKEUP_ENABLE && is_remote_waking_up == false)
               {
                  uint8_t zero[USB_MOUSE_DATA_LEN] = {0};
                  if (0 != memcmp(mouse_data, zero, sizeof(zero)))
                  {
                     APP_PRINT_INFO0("[app_usb_send_mouse_data] usb wakeup");
                     ......
                     if (0 == usb_hid_driver_remote_wakeup(false))
                     {
                        is_usb_allow_enter_dlps = false;
                        is_remote_waking_up = true;
                        ......
                     }
                  }
               }
         }
      }

      return ret;
   }

当USB从suspend状态被唤醒后，会重新进行枚举，进入USB_SUSPENDED状态，并发送一笔数据以触发对端设备亮屏。

按照USB spec规定，需要USB host使能鼠标remote wake up功能后，鼠标才有主动唤醒的能力。当将宏 **FEATURE_SUPPORT_USB_FORCE_WAKE_UP_HOST** 置1后，鼠标在需要唤醒USB host时会进行强制唤醒，不会检查是否被使能了remote wake up功能。

根据上报率调整USB发包间隔
~~~~~~~~~~~~~~~~~~~~~~~~~~

当 :file:`board.h` 中宏 **FEATURE_CHANGE_USB_INTERVAL_FOR_REPORT_RATE** 设置为1时，打开根据上报率调整USB interrupt report interval功能。打开该功能后，当切换上报率后，会切换USB interrupt report interval与上报率匹配，比如上报率为1KHz时，interval会调整到1ms。当鼠标为USB模式时，切换上报率会在 :func:`mouse_report_rate_chang­e_handle` 函数中关闭USB并重新使能，在初始化的过程中，:func:`usb_interface_mouse_init` 函数中会根据当前的上报率设置USB interrupt report interval。

如果鼠标处于2.4G模式，配对或回连成功后会通过 :func:`app_ppt_send_report_rate` 发送当前的上报率给接收器，接收器端USB也会根据上报率调整USB interrupt report interval。

按键
-------

可以通过GPIO或硬件Keyscan两种方案实现按键功能。两种方案在功能和性能上，主要差别如下：

   - 键按下时，Keyscan方案的功耗会稍微高一些：GPIO方案通过触发GPIO中断来检测按键按下和释放，按键按下时也可以进入DLPS；Keyscan方案在按键按下后需要持续扫描，无法进入DLPS（进入DLPS后Keyscan硬件模块会掉电，无法进行扫描），因此按键按下时，Keyscan方案的功耗会稍微高一些。
   - 在IO管脚数量有限时，Keyscan可以提供更多的输入，实现更多的按键。

GPIO按键
~~~~~~~~~~~

GPIO方案需要将 :file:`board.h` 中宏定义 **MOUSE_GPIO_BUTTON_EN** 设置为1。通过读取管脚输入电平判断用户是否按下或释放按键，采用硬件GPIO debounce实现去抖功能。以按键按下检测为例，基本流程为：GPIO初始设定低电平触发中断，当用户按下按键超过配置的GPIO debounce时间，会触发GPIO中断。中断处理函数中，会实时发送键值，并把按键事件发送给app task进行对实时性要求不高的处理（如组合键和功能键等），同时把GPIO设定为高电平触发去检测按键释放。

.. figure:: ../figures/GPIO_key_detection_and_processing_flow.*
   :align: center
   :scale: 80%
   :name: GPIO按键检测和处理流程

   GPIO按键检测和处理流程

管脚配置
^^^^^^^^^^

GPIO按键方案使用的管脚、IRQ以及中断函数在 :file:`board.h` 中配置：

.. code-block:: c

   #define LEFT_BUTTON                     P1_2
   #define LEFT_BUTTON_IRQ                 GPIOA10_IRQn
   #define left_button_int_handler         GPIOA10_Handler

   #define RIGHT_BUTTON                    P2_7
   #define RIGHT_BUTTON_IRQ                GPIOA28_IRQn
   #define right_button_int_handler        GPIOA28_Handler

   #define MID_BUTTON                      P2_3
   #define MID_BUTTON_IRQ                  GPIOA24_IRQn
   #define mid_button_int_handler          GPIOA24_Handler

   #define FORWARD_BUTTON                  P3_4
   #define FORWARD_BUTTON_IRQ              GPIOB1_IRQn
   #define forward_button_int_handler      GPIOB1_Handler

   #define BACK_BUTTON                     P3_3
   #define BACK_BUTTON_IRQ                 GPIOB0_IRQn
   #define back_button_int_handler         GPIOB0_Handler

   #define DPI_BUTTON                      MICBIAS
   #define DPI_BUTTON_IRQ                  GPIOA16_IRQn
   #define dpi_button_int_handler          GPIOA16_Handler

初始化
^^^^^^^^^

以左键为例进行说明。

- PAD初始化：

   .. code-block:: c

      void mouse_gpio_button_module_pad_config(void)
      {
         Pad_Config(LEFT_BUTTON, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
      }

- Pinmux初始化：

   .. code-block:: c

      void mouse_gpio_button_module_pinmux_config(void)
      {
         Pinmux_Config(LEFT_BUTTON, DWGPIO);
      }

- GPIO模块初始化：

   .. code-block:: c

      void mouse_gpio_button_module_init(void)
      {
         APP_PRINT_INFO0("[mouse_gpio_button_module_init] mouse button gpio init");
         RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
         RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);

         APP_PRINT_INFO1("LEFT_BUTTON = %d", GPIO_ReadInputDataBit(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON)));

         GPIO_InitTypeDef GPIO_Param = {0};
         GPIO_StructInit(&GPIO_Param);

         GPIO_Param.GPIO_ITCmd = ENABLE;
         GPIO_Param.GPIO_ITTrigger = GPIO_INT_TRIGGER_LEVEL;
         GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
         /* debounce time = (CntLimit + 1) * DEB_CLK, uint: s*/
         GPIO_Param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
         GPIO_Param.GPIO_DebounceClkSource = GPIO_DEBOUNCE_32K;
         GPIO_Param.GPIO_DebounceClkDiv    = GPIO_DEBOUNCE_DIVIDER_8;
         GPIO_Param.GPIO_DebounceCntLimit = 4 * GPIO_KEY_HW_DEBOUNCE_TIMEOUT - 1;
      }


   重要参数说明：

      1. GPIO_ITCmd：GPIO中断使能或失能。
      2. GPIO_ITTrigger：GPIO中断触发方式，有边沿和电平触发两种方式，推荐使用电平触发（参考方案是基于电平触发实现的）。
      3. GPIO_ITPolarity：GPIO中断触发电平或边沿极性，初始设置为低电平触发。
      4. GPIO_ITDebounce：GPIO debounce使能或失能。
      5. GPIO_DebounceClkSource和GPIO_DebounceClkDiv：配置GPIO debounce的时钟频率，默认设置为32KHz。
      6. GPIO_DebounceCntLimit：GPIO debounce时间配置，可以通过宏定义 **GPIO_KEY_HW_DEBOUNCE_TIMEOUT** 修改，默认设置为8ms。

- NVIC初始化：

   .. code-block:: c

      void mouse_gpio_button_module_nvic_config(void)
      {
         /* LEFT button */
         NVIC_InitTypeDef NVIC_InitStruct = {0};
         NVIC_InitStruct.NVIC_IRQChannel = LEFT_BUTTON_IRQ;
         NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
         NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
         NVIC_Init(&NVIC_InitStruct);
         /* Enable interrupt */
         GPIO_ClearINTPendingBit(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON));
         GPIO_MaskINTConfig(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON), DISABLE);
         GPIO_INTConfig(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON), ENABLE);
      }

按键去抖
^^^^^^^^^^

在初始化中配置GPIO按键debounce实现去抖，但并不是每个管脚都有单独的GPIO debounce，而是一组几个管脚共用一个GPIO debounce。每组管脚只有同时有一个使能debounce，因此需要在硬件设计阶段，合理规划管脚，避免同一组两个管脚都需要GPIO debounce的情况。GPIO debounce分组详情参考HDK中的文档《RTL87x2G_IOPin_Information.xlsx》。

按键状态识别和处理
^^^^^^^^^^^^^^^^^^^^

以左键为例进行说明。

初始化完成后，当左键按下后，会触发GPIO中断，中断处理函数如下：

.. code-block:: c

   void left_button_int_handler(void)
   {
      GPIO_INTConfig(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON), DISABLE);
      GPIO_MaskINTConfig(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON), ENABLE);
      GPIO_ClearINTPendingBit(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON));

      gpio_key_wake_up_interrupt_handler(LEFT_BUTTON);

      GPIO_ClearINTPendingBit(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON));
      GPIO_MaskINTConfig(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON), DISABLE);
      GPIO_INTConfig(GPIO_GetPort(LEFT_BUTTON), GPIO_GetPin(LEFT_BUTTON), ENABLE);

      os_timer_stop(&keys_press_check_timer);
      is_gpio_button_allow_enter_dlps = true;
   }

其中 :mod:`gpio_key_wake_up_interrupt_handler` 中进行：

   1. GPIO中断极性的翻转，以便后续检测按键的释放。
   2. 及时进行BLE/2.4G/USB三种通信模式的按键数据发送。
   3. 会发送消息给app task，在app task中进行对实时性要求不高的处理，如组合键检测。

Keyscan按键
~~~~~~~~~~~~~~

Keyscan方案需要将 :file:`board.h` 中宏定义 **MOUSE_KEYSCAN_EN** 设置为1。Keyscan方案使用硬件Keyscan模块扫描得到按键的按下和释放状态。

管脚配置
^^^^^^^^^^^

Keyscan按键方案使用管脚在 :file:`board.h` 中配置：

.. code-block:: c

   /* if set KEYSCAN_FIFO_LIMIT larger than 3, need to caution ghost key issue */
   #define KEYSCAN_FIFO_LIMIT    3  /* value range from 1 to 26 */

   #define KEYSCAN_ROW_SIZE                3
   #define KEYSCAN_COLUMN_SIZE             3

   #define KEYSCAN_ROW_0                   P2_5
   #define KEYSCAN_ROW_1                   P2_6
   #define KEYSCAN_ROW_2                   P2_7
   #if FEATURE_SUPPORT_KEY_LONG_PRESS_PROTECT
   #define KEYSCAN_ROW_0_IRQ               GPIOA26_IRQn
   #define keyscan_row_0_int_handler       GPIOA26_Handler
   #define KEYSCAN_ROW_1_IRQ               GPIOA27_IRQn
   #define keyscan_row_1_int_handler       GPIOA27_Handler
   #define KEYSCAN_ROW_2_IRQ               GPIOA28_IRQn
   #define keyscan_row_2_int_handler       GPIOA28_Handler
   #endif

   #define KEYSCAN_COLUMN_0                MICBIAS
   #define KEYSCAN_COLUMN_1                P3_0
   #define KEYSCAN_COLUMN_2                P3_1

Keyscan的按键对应关系在 :file:`mouse_keyscan_driver.c` 中定义，可根据实际需求进行修改：

.. code-block:: c

   T_KEY_INDEX_DEF KEY_MAPPING_TABLE[KEYSCAN_ROW_SIZE][KEYSCAN_COLUMN_SIZE] =
   {
      {LEFT_BUTTON_PRESS_MASK_BIT,  INVALID_BUTTON_MASK_BIT,       INVALID_BUTTON_MASK_BIT},
      {MID_BUTTON_PRESS_MASK_BIT,   FORWARD_BUTTON_PRESS_MASK_BIT, DPI_BUTTON_PRESS_MASK_BIT},
      {RIGHT_BUTTON_PRESS_MASK_BIT, BACK_BUTTON_PRESS_MASK_BIT,    INVALID_BUTTON_MASK_BIT},
   };

初始化
^^^^^^^^^

- PAD初始化：

   .. code-block:: c

      void mouse_keyscan_module_pad_config(void)
      {
         Pad_Config(KEYSCAN_ROW_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
         Pad_SetPullStrength(KEYSCAN_ROW_0, PAD_PULL_STRONG);
         Pad_Config(KEYSCAN_ROW_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
         Pad_SetPullStrength(KEYSCAN_ROW_1, PAD_PULL_STRONG);
         Pad_Config(KEYSCAN_ROW_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
         Pad_SetPullStrength(KEYSCAN_ROW_2, PAD_PULL_STRONG);

         Pad_Config(KEYSCAN_COLUMN_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                     PAD_OUT_LOW);
         Pad_Config(KEYSCAN_COLUMN_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                     PAD_OUT_LOW);
         Pad_Config(KEYSCAN_COLUMN_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                     PAD_OUT_LOW);

         keyscan_global_data.is_pinmux_setted = true;
      }

- Pinmux初始化：

   .. code-block:: c

      void mouse_keyscan_module_pinmux_config(void)
      {
         Pinmux_Config(KEYSCAN_ROW_0, KEY_ROW_0);
         Pinmux_Config(KEYSCAN_ROW_1, KEY_ROW_1);
         Pinmux_Config(KEYSCAN_ROW_2, KEY_ROW_2);

         Pinmux_Config(KEYSCAN_COLUMN_0, KEY_COL_0);
         Pinmux_Config(KEYSCAN_COLUMN_1, KEY_COL_1);
         Pinmux_Config(KEYSCAN_COLUMN_2, KEY_COL_2);
      }

- Keyscan模块初始化：

   .. code-block:: c

      void mouse_keyscan_module_init(KEYSCANScanMode_TypeDef ScanMode,
                                    FunctionalState vDebounce_En, uint32_t DebounceTime,
                                    FunctionalState vScantimerEn, uint32_t ScanInterval)
      {
         if (false == keyscan_global_data.is_pinmux_setted)
         {
            mouse_keyscan_module_pad_config();
         }
         RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, DISABLE);
         RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, ENABLE);

         KEYSCAN_InitTypeDef KEYSCAN_InitStruct = {0};
         KeyScan_StructInit(&KEYSCAN_InitStruct);

         KEYSCAN_InitStruct.rowSize       = KEYSCAN_ROW_SIZE;
         KEYSCAN_InitStruct.colSize       = KEYSCAN_COLUMN_SIZE;
         KEYSCAN_InitStruct.scanmode      = ScanMode;

         KEYSCAN_InitStruct.clockdiv      = 0x26;  /* 128kHz = 5MHz/(clockdiv+1) */
         KEYSCAN_InitStruct.delayclk      = 0x0f;  /* 8kHz = 5MHz/(clockdiv+1)/(delayclk+1) */
         KEYSCAN_InitStruct.debounceEn    = vDebounce_En;
         KEYSCAN_InitStruct.scantimerEn   = vScantimerEn;
         KEYSCAN_InitStruct.debouncecnt   = DebounceTime * 8;  /* DebounceCnt = DebounceTime * 8kHz */
         KEYSCAN_InitStruct.scanInterval  = ScanInterval * 8;  /* IntervalCnt = ScanInterval * 8kHz */
         KEYSCAN_InitStruct.keylimit      = KEYSCAN_FIFO_LIMIT;
         if (ScanMode == KeyScan_Manual_Scan_Mode)
         {
            KEYSCAN_InitStruct.manual_sel = KeyScan_Manual_Sel_Bit;
            KEYSCAN_InitStruct.detecttimerEn = DISABLE;
         }
         else if (ScanMode == KeyScan_Auto_Scan_Mode)
         {
            KEYSCAN_InitStruct.detecttimerEn = ENABLE;
            KEYSCAN_InitStruct.releasecnt    = KEYSCAN_ALL_RELEASE_TIME *
                                                8;  /* releasecnt = ScanInterval * 8kHz */;
         }

         KeyScan_Init(KEYSCAN, &KEYSCAN_InitStruct);
         KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_SCAN_END | KEYSCAN_INT_ALL_RELEASE);
         KeyScan_INTConfig(KEYSCAN, KEYSCAN_INT_SCAN_END | KEYSCAN_INT_ALL_RELEASE, ENABLE);
         KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END | KEYSCAN_INT_ALL_RELEASE, DISABLE);
         KeyScan_Cmd(KEYSCAN, ENABLE);
      }

   :func:`mouse_keyscan_module_init` 接口有三个参数，ScanMode, DebounceTime和ScanInterval。Keyscan方案使用的ScanMode是KeyScan_Auto_Scan_Mode，当有任意按键被按下时间超过 DebounceTime 后会触发自动扫描，两次扫描的间隔为ScanInterval。Keyscan使用了两种中断：KEYSCAN_INT_SCAN_END中断表示单次扫描结束，以获取自动扫描的结果；KEYSCAN_INT_ALL_RELEASE中断表示全部按键都被释放了。

- NVIC初始化：

   .. code-block:: c

      void mouse_keyscan_module_nvic_config(void)
      {
         NVIC_InitTypeDef NVIC_InitStruct = {0};
         NVIC_InitStruct.NVIC_IRQChannel         = KEYSCAN_IRQn;
         NVIC_InitStruct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
         NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
         NVIC_Init(&NVIC_InitStruct);
      }

按键去抖机制
^^^^^^^^^^^^^^

Keyscan方案debounce分为三种场景：

   1. 第一个按键按下的debounce：使用的是Keyscan模块的硬件debounce，通过 :func:`mouse_keyscan_module_init` 接口参数 DebounceTime 设置。
   2. 已有按键被按下后，后续按键按下和抬起的debounce：当连续n+1次自动扫描（对应n*Scan Interval时间）都扫描到按键状态改变后，才认为检测到新的按键状态，以此实现后续按键的按下和抬起的去抖。
   3. 全部按键都释放的debounce：通过配置 KEYSCAN_INT_ALL_RELEASE 中断的参数 KEYSCAN_ALL_RELEASE_TIME 实现。

Keyscan interval和三种场景的debounce可以通过宏进行统一配置。

按键状态检查和处理
^^^^^^^^^^^^^^^^^^^^

Keyscan的检测流程分为 KEYSCAN_INT_SCAN_END 和 KEYSCAN_INT_ALL_RELEASE 中断相关处理，其中 KEYSCAN_INT_ALL_RELEASE 用于检测是否全部按键都被释放了，KEYSCAN_INT_SCAN_END用于检测按键的其他状态的改变。

基于Keyscan模块的硬件去抖功能，当首个按键按下超过debounce时间后才开始扫描，因此当第一次扫描结束触发了 KEYSCAN_INT_FLAG_SCAN_END 中断时，可将扫到的键值判定为有效按键；如果不是第一次扫描，则只有当按键状态已经维持了debounce时间后才认为按键状态改变。具体流程为：先判断此次扫描所得键值是否异常超出FIFO大小，超出则重新初始化等待下一次扫描；再判断当前键值是否与记录的上一次扫描值相同，只有当前后值不同并且all release flag为true时才会认为是首个按键，之后每次按键都会再连续扫描KEYSCAN_DEBOUNCE_NUM次，每次的值相同之后才会认为是有效按键。其中宏 **KEYSCAN_DEBOUNCE_NUM** 是根据 KEYSCAN_INTERVAL 和 KEYSCAN_DEBOUNCE 计算得到(KEYSCAN_DEBOUNCE_NUM = (KEYSCAN_DEBOUNCE / KEYSCAN_INTERVAL) + 1)。

.. figure:: ../figures/The_processing_for_scan_end_interrupt.*
   :align: center
   :scale: 80%
   :name: Scan End中断相关处理

   Scan End中断相关处理

全部按键都被释放后会触发 KEYSCAN_INT_ALL_RELEASE 中断，将会进行按键释放处理并重新初始化keyscan模块。

.. figure:: ../figures/The_processing_for_all_release_interrupt.*
   :align: center
   :scale: 80%
   :name: All Release中断相关处理

   All Release中断相关处理

在被判断为有效按键之后，将通过 :func:`keyscan_event_handler` 进行处理。具体流程为：先把传入的按键值，与保存的按键值进行比较，若值相同则不作处理直接返回，避免了重复向对端发送键值；若与保存值不同，则向对端发送键值，同时会发消息到app task处理组合键和功能键等。

长按键保护
^^^^^^^^^^^^

在 :file:`board.h` 中宏 **FEATURE_SUPPORT_KEY_LONG_PRESS_PROTECT** 默认设置为1，打开长按键保护功能。长按键保护的检测时间通过宏 **LONG_PRESS_KEY_DETECT_TIMEOUT** 进行修改，默认是30秒。

.. code-block:: c

   #define FEATURE_SUPPORT_KEY_LONG_PRESS_PROTECT     1  /* set 1 to stop scan when press one key too long */

   #if FEATURE_SUPPORT_KEY_LONG_PRESS_PROTECT
   #define LONG_PRESS_KEY_DETECT_TIMEOUT              30000 /* 30 sec */
   #endif

打开长按键保护后，每次按键按下都会复位 :mod:`long_press_key_detect_timer`，当一个或多个按键按下并超过一定时间未释放时，会在 :mod:`long_press_key_detect_timer_cb` 中把keyscan各个行的管脚配置为GPIO，并把中断方式设为高电平触发，有按键按下的行管脚将使能中断，此时可以进入DLPS。

.. code-block:: c

   static void long_press_key_detect_timer_cb(TimerHandle_t p_timer)
   {
      APP_PRINT_INFO0("[long_press_key_detect_timer_cb] detect key long pressed event");
      keyscan_global_data.is_key_long_pressed = true;
      KeyScan_Cmd(KEYSCAN, DISABLE);

      /* reset row pins for gpio config */
      Pinmux_Config(KEYSCAN_ROW_0, DWGPIO);
      Pinmux_Config(KEYSCAN_ROW_1, DWGPIO);
      Pinmux_Config(KEYSCAN_ROW_2, DWGPIO);

      RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
      RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
      GPIO_InitTypeDef GPIO_Param = {0};
      GPIO_StructInit(&GPIO_Param);
      GPIO_Param.GPIO_ITCmd = ENABLE;
      GPIO_Param.GPIO_ITTrigger = GPIO_INT_TRIGGER_LEVEL;
      GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;

      NVIC_InitTypeDef NVIC_InitStruct = {0};
      NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
      NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;

      APP_PRINT_INFO1("long pressed keys are 0x%2X",
                     app_global_data.mouse_current_data.button);

      uint8_t row_button_mask[KEYSCAN_ROW_SIZE] = {0};
      for (uint8_t i = 0; i < KEYSCAN_ROW_SIZE; i++)
      {
         for (uint8_t j = 0; j < KEYSCAN_COLUMN_SIZE; j++)
         {
               row_button_mask[i] |= KEY_MAPPING_TABLE[i][j];
         }
      }

      if (app_global_data.mouse_current_data.button & row_button_mask[0])
      {
         /* row 0 gpio */
         GPIO_Param.GPIO_Pin = GPIO_GetPin(KEYSCAN_ROW_0);
         GPIO_Init(GPIO_GetPort(KEYSCAN_ROW_0), &GPIO_Param);
         /* row 0 nvic */
         NVIC_InitStruct.NVIC_IRQChannel = KEYSCAN_ROW_0_IRQ;
         NVIC_Init(&NVIC_InitStruct);
         /* enable row 0 interrupt */
         GPIO_ClearINTPendingBit(GPIO_GetPort(KEYSCAN_ROW_0), GPIO_GetPin(KEYSCAN_ROW_0));
         GPIO_MaskINTConfig(GPIO_GetPort(KEYSCAN_ROW_0), GPIO_GetPin(KEYSCAN_ROW_0), DISABLE);
         GPIO_INTConfig(GPIO_GetPort(KEYSCAN_ROW_0), GPIO_GetPin(KEYSCAN_ROW_0), ENABLE);

         is_row_0_long_pressed = true;
      }
      ......

当长按键全部被松开后，将触发GPIO中断，并把各管脚重新配置到keyscan模块并重新初始化。

.. code-block:: c

   void keyscan_row_0_int_handler(void)
   {
      GPIO_INTConfig(GPIO_GetPort(KEYSCAN_ROW_0), GPIO_GetPin(KEYSCAN_ROW_0), DISABLE);
      GPIO_MaskINTConfig(GPIO_GetPort(KEYSCAN_ROW_0), GPIO_GetPin(KEYSCAN_ROW_0), ENABLE);
      GPIO_ClearINTPendingBit(GPIO_GetPort(KEYSCAN_ROW_0), GPIO_GetPin(KEYSCAN_ROW_0));

      APP_PRINT_INFO0("[keyscan_row_0_int_handler] long pressed row_0 keys release");
      is_row_0_long_pressed = false;

      if (is_row_1_long_pressed == false && is_row_2_long_pressed == false)
      {
         APP_PRINT_INFO0("[keyscan_row_0_int_handler] long pressed keys all release");
         keyscan_global_data.is_key_long_pressed = false;

         if (keyscan_global_data.is_all_key_released == false)
         {
               keyscan_global_data.is_all_key_released = true;
               memset(&keyscan_global_data.cur_fifo_data, 0, sizeof(T_KEYSCAN_FIFO_DATA));
               keyscan_event_handler(&keyscan_global_data.cur_fifo_data);
         }

         mouse_keyscan_init_data();
         Pinmux_Config(KEYSCAN_ROW_0, KEY_ROW_0);
         Pinmux_Config(KEYSCAN_ROW_1, KEY_ROW_1);
         Pinmux_Config(KEYSCAN_ROW_2, KEY_ROW_2);
         mouse_keyscan_module_init(KeyScan_Auto_Scan_Mode, ENABLE, KEYSCAN_DEBOUNCE, ENABLE,
                                    KEYSCAN_INTERVAL);
      }
   }

DPI按键处理
~~~~~~~~~~~~~

DPI按键按下并释放后，会循环切换到下一档位的DPI设置并有灯效提示。

- DPI档位如下：

.. code-block:: c

   /* dpi range:50~26000 , corresponding to 0x0000--- 0x0207 */
   #define DPI_LEVEL1                      800    /* 800dpi */
   #define DPI_LEVEL2                      1200   /* 1200dpi */
   #define DPI_LEVEL3                      1600   /* 1600dpi */
   #define DPI_LEVEL4                      3200   /* 3200dpi */
   #define DPI_LEVEL5                      4800   /* 4800dpi */

   /*DEFAULT_DPI_LEVEL value is 1-6*/
   #define DPI_INDEX_MIN                   1
   #define DPI_INDEX_MAX                   5
   #define DEFAULT_DPI_INDEX               2

- DPI按键处理如下：

.. code-block:: c

   static void mouse_dpi_key_release_event(void)
   {
      if (app_global_data.mode_type == USB_MODE ||
         ppt_app_global_data.mouse_ppt_status == MOUSE_PPT_STATUS_CONNECTED ||
         app_global_data.mouse_ble_status == MOUSE_BLE_STATUS_PAIRED)
      {
   #if PAW3395_SENSOR_EN
         uint16_t dpi_value;
         paw3395_global_data.dpi_level++;

         if (paw3395_global_data.dpi_level > DPI_INDEX_MAX)
         {
               paw3395_global_data.dpi_level = DPI_INDEX_MIN;
         }
         dpi_value = paw3395_get_dpi_value_by_index(paw3395_global_data.dpi_level - 1);
         paw3395_module_dpi_config(dpi_value, dpi_value);
         if (ftl_save_to_module("app", &paw3395_global_data.dpi_level, FTL_DPI_OFFSET, FTL_DPI_LEN))
         {
               APP_PRINT_ERROR0("[mouse_dpi_key_release_event] ftl_save fail");
         }
         if (ppt_app_global_data.mouse_ppt_status == MOUSE_PPT_STATUS_CONNECTED)
         {
               app_ppt_send_dpi_data(SYNC_MSG_TYPE_DYNAMIC_RETRANS, 0);
         }
   #if SUPPORT_LED_INDICATION_FEATURE
         dpi_led_indication(paw3395_global_data.dpi_level);
   #endif
   #endif
      }
   }

Report rate组合键处理
~~~~~~~~~~~~~~~~~~~~~~~

BLE模式下受限于BLE connection interval，上报率固定在125Hz。2.4G和USB模式下，长按Report rate组合键 :kbd:`中+前进+后退` 3秒，会循环切换到下一档位的上报率。

- 上报率档位：

.. code-block:: c

   #define USB_REPORT_RATE_LEVEL_0             1000
   #define USB_REPORT_RATE_LEVEL_1             4000
   #define USB_REPORT_RATE_LEVEL_2             8000
   #define USB_REPORT_RATE_LEVEL_NUM           3

   #define PPT_REPORT_RATE_LEVEL_0             1000
   #define PPT_REPORT_RATE_LEVEL_1             2000
   #define PPT_REPORT_RATE_LEVEL_2             4000
   #define PPT_REPORT_RATE_LEVEL_NUM           3

   #define USB_REPORT_RATE_DEFAULT_INDEX       2
   #define PPT_REPORT_RATE_DEFAULT_INDEX       2

- Report rate按键处理可参考 :func:`mouse_report_rate_change_handle`。

滚轮
-------

鼠标滚轮功能是基于aon qdecoder（简称QDEC）模块实现的，该模块在DLPS下可以正常工作，不会掉电。

管脚配置
~~~~~~~~~~~

滚轮模块所使用的管脚在 :file:`board.h` 中定义，默认如下：

.. code-block:: c

   #define QDEC_X_PHA_PIN                  P9_1
   #define QDEC_X_PHB_PIN                  P9_0

.. note:: 
   只有固定的几个管脚可以映射到滚轮模块使用，详情参考datasheet。

初始化
~~~~~~~~~

- PAD初始化：

   .. code-block:: c

      void qdec_module_pad_config(void)
      {
         Pad_Config(QDEC_X_PHA_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                     PAD_OUT_LOW);
         Pad_Config(QDEC_X_PHB_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                     PAD_OUT_LOW);
      }

- Pinmux初始化：

   .. code-block:: c

      void qdec_module_pinmux_config(void)
      {
         Pinmux_AON_Config(QDPH0_IN_P9_0_P9_1);
      }

- 滚轮模块初始化配置如下：

   .. code-block:: c

      void qdec_cfg_init(uint32_t is_debounce, uint8_t phasea, uint8_t phaseb)
      {
         AON_QDEC_InitTypeDef qdecInitStruct = {0};
         AON_QDEC_StructInit(&qdecInitStruct);
         qdecInitStruct.debounceTimeX =
            20;/* uint 1/32 ms, recommended debounce time setting is between 600us and 1000us */
         qdecInitStruct.axisConfigX = ENABLE;
         qdecInitStruct.debounceEnableX = is_debounce;
         qdecInitStruct.initPhaseX = (phasea << 1) | phaseb;
         qdecInitStruct.manualLoadInitPhase = ENABLE;
         qdecInitStruct.counterScaleX = CounterScale_2_Phase;

         AON_QDEC_Init(AON_QDEC, &qdecInitStruct);

         AON_QDEC_INTConfig(AON_QDEC, AON_QDEC_X_INT_NEW_DATA, ENABLE);
         AON_QDEC_INTConfig(AON_QDEC, AON_QDEC_X_INT_ILLEAGE, ENABLE);
         AON_QDEC_INTMask(AON_QDEC, AON_QDEC_X_INT_MASK, DISABLE);
         AON_QDEC_INTMask(AON_QDEC, AON_QDEC_X_CT_INT_MASK, DISABLE);
         AON_QDEC_INTMask(AON_QDEC, AON_QDEC_X_ILLEAGE_INT_MASK, DISABLE);
         AON_QDEC_Cmd(AON_QDEC, AON_QDEC_AXIS_X, ENABLE);

         GPIO_InitTypeDef GPIO_InitStruct = {0};
         GPIO_StructInit(&GPIO_InitStruct);
         GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(QDEC_X_PHA_PIN);
         GPIO_InitStruct.GPIO_Mode   = GPIO_MODE_IN;
         GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
         GPIO_Init(GPIO_GetPort(QDEC_X_PHA_PIN), &GPIO_InitStruct);
         GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(QDEC_X_PHB_PIN);
         GPIO_Init(GPIO_GetPort(QDEC_X_PHB_PIN), &GPIO_InitStruct);
      }

   其中重要参数说明：

      1. axisConfigX：QDEC功能使能，需要配置为ENABLE。
      2. debounceEnableX：debounce使能，根据需要配置为 ENABLE 或 DISABLE。
      3. debounceTimeX：debounce时间，单位为1/32 ms，根据时间需求设置。
      4. initPhaseX：初始相位设置。
      5. manualLoadInitPhase：自动加载相位功能，需要设置为ENABLE，每次产生中断后，会自动加载当前相位。
      6. counterScaleX：表示多少个相位变化会进行计数和产生中断，设置为CounterScale_2_Phase。

- NVIC初始化如下：

   .. code-block:: c

      void qdec_module_nvic_config(void)
      {
         NVIC_InitTypeDef nvic_init_struct = {0};
         nvic_init_struct.NVIC_IRQChannel         = AON_QDEC_IRQn;
         nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
         nvic_init_struct.NVIC_IRQChannelPriority = 3;
         NVIC_Init(&nvic_init_struct);
      }

- 取消初始化如下：

   .. code-block:: c

      void qdec_module_deinit(void)
      {
         QDEC_DBG_BUFFER(MODULE_APP, LEVEL_INFO, "qdec deinit", 0);
         AON_QDEC_Cmd(AON_QDEC, AON_QDEC_AXIS_X, DISABLE);
      }

状态获取和数据发送
~~~~~~~~~~~~~~~~~~~~

滚轮模块基于硬件的AON QDEC（DLPS下不掉电），通过QDEC两个管脚的相位变化（即电平变化）来获取滚轮的状态。配置了两种中断 AON_QDEC_FLAG_NEW_CT_STATUS_X 和 AON_QDEC_FLAG_ILLEGAL_STATUS_X，前者为滚轮模块正常检查到了2个相位的变化所触发的（比如相位依次变化：00到01/10到11），后者表示没有发现2个相位的依次变化，而是检查到了相位的突变（比如00到11）。

在QDEC中断处理函数中，会先获取QDEC两个管脚当前的电平。根据两个管脚的电平判断当前滚轮是否滚动的一格；如果是从DLPS唤醒起来，也会根据DLPS唤醒管脚的情况来辅助判断。由于滚轮滚动半格再滚回原位，相位从00变化01/10变化回00，也是变化了2个相位，也会触发 AON_QDEC_FLAG_NEW_CT_STATUS_X 中断，因此需要通过滚轮管脚的状态来判断滚轮的滚动状态。而后读取QDEC的滚动方向并进行数据处理，如果当前处于USB模式、2.4G连接状态或BLE连接状态，会在中断处理函数中直接发送数据，保证实时性；否则就发送消息给app task处理，进行回连等操作。

.. figure:: ../figures/QDEC_interrupt_handler.*
   :align: center
   :scale: 80%
   :name: QDEC中断处理函数

   QDEC中断处理函数

光学传感器
------------

本文以SDK中使用的PAW3395做为参考设计，开发者可按照产品需求选择适合的Sensor。

管脚配置
~~~~~~~~~~

PAW3395所使用的管脚在 :file:`board.h` 中定义，包括SPI通信相关的管脚和表征PAW3395有无数据的Motion Pin。

.. code-block:: c

   #define SENSOR_SPI_CLK                  P4_0
   #define SENSOR_SPI_MISO                 P4_1
   #define SENSOR_SPI_MOSI                 P4_2
   #define SENSOR_SPI_NCS                  P4_3
   #define SENSOR_SPI_NRESET               P0_5

   #define SENSOR_SPI_MOTION               P0_6
   #define SENSOR_MOTION_IRQ               GPIOA6_IRQn
   #define mouse_motion_pin_handler        GPIOA6_Handler

初始化
~~~~~~~~~

PAW3395模块的初始化内容包括：PAD和Pinmux初始化，SPI初始化，PAW3395初始化（DPI等），Motion检测初始化，上报率和采样定时器初始化。

PAD和Pinmux初始化
^^^^^^^^^^^^^^^^^^^

- PAD初始化：

.. code-block:: c

   void paw3395_module_pad_config(void)
   {
      Pad_Config(SENSOR_SPI_CLK,  PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                  PAD_OUT_HIGH);
      Pad_Config(SENSOR_SPI_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                  PAD_OUT_HIGH);
      Pad_Config(SENSOR_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                  PAD_OUT_HIGH);
      Pad_Config(SENSOR_SPI_NCS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
                  PAD_OUT_HIGH);
      Pad_Config(SENSOR_SPI_MOTION, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                  PAD_OUT_HIGH);
      Pad_Config(SENSOR_SPI_NRESET, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE,
                  PAD_OUT_HIGH);
   }

- Pinmux初始化：

.. code-block:: c

   void paw3395_module_pinmux_config(void)
   {
      Pinmux_Deinit(SENSOR_SPI_CLK);
      Pinmux_Deinit(SENSOR_SPI_MISO);
      Pinmux_Deinit(SENSOR_SPI_MOSI);
      Pinmux_Deinit(SENSOR_SPI_NCS);

      Pinmux_Config(SENSOR_SPI_CLK,  SPI0_CLK_MASTER);
      Pinmux_Config(SENSOR_SPI_MISO, SPI0_MI_MASTER);
      Pinmux_Config(SENSOR_SPI_MOSI, SPI0_MO_MASTER);
      Pinmux_Config(SENSOR_SPI_NCS, DWGPIO);
      Pinmux_Config(SENSOR_SPI_MOTION, DWGPIO);
   }

SPI初始化
^^^^^^^^^^^

SPI初始化配置如下，其中SPI Clock设置为10MHz。

.. code-block:: c

   void paw3395_module_master_spi_init(void)
   {
      RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

      SPI_InitTypeDef  SPI_InitStruct = {0};
      SPI_StructInit(&SPI_InitStruct);

      SPI_InitStruct.SPI_Direction   = SPI_Direction_FullDuplex;
      SPI_InitStruct.SPI_DataSize    = SPI_DataSize_8b;
      SPI_InitStruct.SPI_CPOL        = SPI_CPOL_High;
      SPI_InitStruct.SPI_CPHA        = SPI_CPHA_2Edge;
      SPI_InitStruct.SPI_BaudRatePrescaler  = SPI_BaudRatePrescaler_4;
      SPI_InitStruct.SPI_FrameFormat = SPI_Frame_Motorola;
      SPI_Init(PAW3395_SPI, &SPI_InitStruct);
      SPI_Cmd(PAW3395_SPI, ENABLE);

      RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
      RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
      GPIO_InitTypeDef GPIO_InitStruct = {0};
      GPIO_StructInit(&GPIO_InitStruct);
      GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(SENSOR_SPI_NCS);
      GPIO_InitStruct.GPIO_Mode   = GPIO_MODE_OUT;
      GPIO_Init(GPIO_GetPort(SENSOR_SPI_NCS), &GPIO_InitStruct);
      GPIO_SetBits(GPIO_GetPort(SENSOR_SPI_NCS), GPIO_GetPin(SENSOR_SPI_NCS));
   }

PAW3395初始化
^^^^^^^^^^^^^^^

PAW3395初始化主要包括DPI初始化，XY轴方向初始化，工作模式初始化。

DPI初始化会从flash的FTL区域获取保存的index值，如果没有保存过，则设置为默认的DPI index，并存储到FTL中。其中FTL区域详细说明请参考文档 \ :Doc:`Memory <../../../../doc/platform/memory/text_cn/README>`\。

DPI初始化代码如下：

.. code-block:: c

   void paw3395_dpi_init(void)
   {
      uint16_t x_axis_dpi = 0;
      if (0 != ftl_load_from_module("app", &paw3395_global_data.dpi_level, FTL_DPI_OFFSET, FTL_DPI_LEN))
      {
         paw3395_global_data.dpi_level = DEFAULT_DPI_INDEX;
         ftl_save_to_module("app", &paw3395_global_data.dpi_level, FTL_DPI_OFFSET, FTL_DPI_LEN);
      }
      else
      {
         if (paw3395_global_data.dpi_level < DPI_INDEX_MIN ||
               paw3395_global_data.dpi_level > DPI_INDEX_MAX)
         {
            paw3395_global_data.dpi_level = DEFAULT_DPI_INDEX;
            ftl_save_to_module("app", &paw3395_global_data.dpi_level, FTL_DPI_OFFSET, FTL_DPI_LEN);
         }
      }
      x_axis_dpi = paw3395_get_dpi_value_by_index(paw3395_global_data.dpi_level - 1);
      paw3395_module_dpi_config(x_axis_dpi, x_axis_dpi);
   }

XY轴方向初始化，是需要根据实际sensor物理摆放位置，调整X轴和Y轴的位置和方向：

.. code-block:: c

   /* set sensor axis control */
   paw3395_write_reg(REG_AXIS_CONTROL, 0x40);

PAW3395有多种工作模式，需要根据鼠标的传输模式和上报率进行选择。

Motion检测初始化
^^^^^^^^^^^^^^^^^^

Motion Pin默认状态是高电平，当PAW3395有数据产生，Motion Pin会变为低电平，通过设置Motion Pin的GPIO中断，来检测鼠标是否有移动：

.. code-block:: c

   void paw3395_module_motion_gpio_init(void)
   {
      RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
      RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);

      GPIO_InitTypeDef GPIO_InitStruct = {0};
      GPIO_StructInit(&GPIO_InitStruct);
      GPIO_InitStruct.GPIO_Pin  = GPIO_GetPin(SENSOR_SPI_MOTION);
      GPIO_InitStruct.GPIO_ITCmd = ENABLE;
      GPIO_InitStruct.GPIO_ITTrigger = GPIO_INT_TRIGGER_LEVEL;
      GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
      GPIO_Init(GPIO_GetPort(SENSOR_SPI_MOTION), &GPIO_InitStruct);
   }

上报率和采样定时器初始化
^^^^^^^^^^^^^^^^^^^^^^^^^

鼠标BLE模式上报率固定为125Hz，2.4G和USB模式的上报率初始化会从flash的FTL区域获取保存的Report rate index值，如果没有保存过，则使用默认值，并存储到FTL区域中。上报率会根据USB的通信速率做限制，如USB模式如果使用了Full speed，则上报率不能超过1KHz，防止出现采样频率超过传输速率而导致丢包。上报率相关的初始化在 :file:`app_init_global_data` 中。

.. code-block:: c

   void app_init_global_data(void)
   {
      memset(&app_global_data, 0, sizeof(app_global_data));
      app_global_data.mtu_size = 23;
      app_global_data.max_report_rate_level = REPORT_RATE_LEVEL_8000_HZ;
      if (0 != ftl_load_from_module("app", &app_global_data.usb_report_rate_index,
                                    FTL_USB_REPORT_RATE_INDEX_OFFSET,
                                    FTL_USB_REPORT_RATE_INDEX_LEN))
      {
         app_global_data.usb_report_rate_index = USB_REPORT_RATE_DEFAULT_INDEX;
         ftl_save_to_module("app", &app_global_data.usb_report_rate_index, FTL_USB_REPORT_RATE_INDEX_OFFSET,
                              FTL_USB_REPORT_RATE_INDEX_LEN);
      }
      if (0 != ftl_load_from_module("app", &app_global_data.ppt_report_rate_index,
                                    FTL_PPT_REPORT_RATE_INDEX_OFFSET,
                                    FTL_PPT_REPORT_RATE_INDEX_LEN))
      {
         app_global_data.ppt_report_rate_index = PPT_REPORT_RATE_DEFAULT_INDEX;
         ftl_save_to_module("app", &app_global_data.ppt_report_rate_index, FTL_PPT_REPORT_RATE_INDEX_OFFSET,
                              FTL_PPT_REPORT_RATE_INDEX_LEN);
      }
   }

Sensor采样定时器会根据上报率来确定初始化参数：

.. code-block:: c

   if (app_global_data.mode_type == BLE_MODE)
   {
      paw3395_module_sample_timer_init(BLE_SAMPLE_PERIOD);
   }
   else if (app_global_data.mode_type == PPT_2_4G)
   {
      paw3395_global_data.sample_period = (SAMPLE_CLOCK_SOURCE / get_report_rate_level_by_index(PPT_2_4G,
                                             app_global_data.ppt_report_rate_index, app_global_data.max_report_rate_level)) - 1;
      paw3395_module_sample_timer_init(paw3395_global_data.sample_period);
   }
   else if (app_global_data.mode_type == USB_MODE)
   {
      paw3395_global_data.sample_period = (SAMPLE_CLOCK_SOURCE / get_report_rate_level_by_index(USB_MODE,
                                             app_global_data.usb_report_rate_index, app_global_data.max_report_rate_level)) - 1;
      paw3395_module_sample_timer_init(paw3395_global_data.sample_period);
   }

移动触发和定时采样
~~~~~~~~~~~~~~~~~~~

鼠标系统初始化完成后，会将Motion Pin GPIO中断使能，当鼠标移动时会触发Motion中断，在中断处理函数 :func:`mouse_motion_pin_handler` 中会关闭Motion中，并打开Sensor采样定时器，根据初始化设置好的时间，定时触发定时器中断。在定时器中断处理函数 :func:`mouse_sample_hw_timer_handler` 中，获取PAW3395的相关寄存器的值（X,Y等）。当有XY数据需要发送时，且处于USB模式、2.4G连接状态或BLE配对状态时，会在中断处理函数中直接发送数据，保证实时性；否则发送消息给app task处理。为了避免Motion中断频繁触发，以及保证数据采样的及时性，如果没有XY数据需要发送，不会马上停止采样定时器，当持续一小段时间没有数据才停止定时器，重新使能Motion管脚的GPIO中断。

电量检测和充电
----------------

USB插拔检测
~~~~~~~~~~~~~

USB_MODE_MONITOR管脚默认是低电平，当USB插入后USB_MODE_MONITOR管脚会被拉高。通过检测USB_MODE_MONITOR管脚的电平来判断USB当前是插入还是拔出状态。USB_MODE_MONITOR管脚配置为GPIO，使能GPIO中断，当USB_MODE_MONITOR管脚的GPIO中断触发后，会打开软件定时器，反复检测若干次电平，来实现去抖动，以防止误判USB插拔状态，具体处理可参考 :func:`usb_mode_monitor_debounce_timeout_cb`。

USB_MODE_MONITOR管脚GPIO中断处理如下：

.. code-block:: c

   void usb_mode_monitor_int_handler(void)
   {
      APP_PRINT_INFO0("usb_mode_monitor_int_handler");
      /*  Mask GPIO interrupt */
      GPIO_INTConfig(GPIO_GetPort(USB_MODE_MONITOR), GPIO_GetPin(USB_MODE_MONITOR), DISABLE);
      GPIO_MaskINTConfig(GPIO_GetPort(USB_MODE_MONITOR), GPIO_GetPin(USB_MODE_MONITOR), ENABLE);
      GPIO_ClearINTPendingBit(GPIO_GetPort(USB_MODE_MONITOR), GPIO_GetPin(USB_MODE_MONITOR));

      if (os_timer_start(&usb_mode_monitor_debounce_timer) == false)
      {
         GPIO_ClearINTPendingBit(GPIO_GetPort(USB_MODE_MONITOR), GPIO_GetPin(USB_MODE_MONITOR));
         GPIO_MaskINTConfig(GPIO_GetPort(USB_MODE_MONITOR), GPIO_GetPin(USB_MODE_MONITOR), DISABLE);
         GPIO_INTConfig(GPIO_GetPort(USB_MODE_MONITOR), GPIO_GetPin(USB_MODE_MONITOR), ENABLE);
      }
      else if (usb_mode_monitor_trigger_level == GPIO_PIN_LEVEL_HIGH)
      {
         is_usb_in_debonce_check = true;
         usb_in_debonce_timer_num = 0;
      }
      else
      {
         is_usb_out_debonce_check = true;
         usb_out_debonce_timer_num = 0;
      }
   }

电池电量获取
~~~~~~~~~~~~~~

电池电压会通过外部电路分压到0.9V以下，然后通过ADC进行采样。采样得到电压值后，通过换算得到电池电压和电量百分比。

.. figure:: ../figures/Battery_voltage_divider_and_ADC_sampling_circuit.*
   :align: center
   :scale: 60%
   :name: 电池电压分压和ADC采样电路

   电池电压分压和ADC采样电路

管脚配置
^^^^^^^^^^

ADC采样所使用的管脚在 :file:`board.h` 中定义，ADC管脚只能使用 P2_0 ~ P2_7，对应 ADC0 ~ ADC7。

.. code-block:: c

   #define BAT_ADC_PIN                             ADC_5
   #define ADC_SAMPLE_CHANNEL                      5

ADC初始化
^^^^^^^^^^^

ADC配置为bypass mode，使用one shot采样。

.. code-block:: c

   void bat_init_adc(void)
   {
      /* adc init */
      Pad_Config(BAT_ADC_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                  PAD_OUT_LOW);
      Pinmux_Config(BAT_ADC_PIN, IDLE_MODE);

      ADC_DeInit(ADC);
      RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

      ADC_InitTypeDef adcInitStruct = {0};
      ADC_StructInit(&adcInitStruct);

      for (uint8_t index = 0; index < BAT_ADC_SAMPLE_CNT; index++)
      {
         adcInitStruct.ADC_SchIndex[index] = EXT_SINGLE_ENDED(ADC_SAMPLE_CHANNEL);
      }
      adcInitStruct.ADC_Bitmap                = (1 << BAT_ADC_SAMPLE_CNT) - 1;
      adcInitStruct.ADC_SampleTime            = 14; /* sample time 1.5us */

      ADC_Init(ADC, &adcInitStruct);
      ADC_BypassCmd(ADC_SAMPLE_CHANNEL, ENABLE);
      ADC_INTConfig(ADC, ADC_INT_ONE_SHOT_DONE, ENABLE);
   }

电量获取和换算
^^^^^^^^^^^^^^^^

宏定义 **SUPPORT_BAT_PERIODIC_DETECT_FEATURE** 设置为1后，会打开软件定时器 :mod:`bat_detect_timer`，定时进行ADC采样和换算获取电池电压，具体处理可参考 :func:`bat_detect_battery_mv`。

获得电池电压后，根据电池放电和充电的两个电压电量对应表 :mod:`bat_vol_discharge_level_table` 和 :mod:`bat_vol_charge_level_table`，查表得到电池电量，具体处理可参考 :func:`bat_calculate_bat_level`。

电量模式
~~~~~~~~~~

鼠标的所有电量模式如下：

.. code-block:: c

   typedef enum
   {
      BAT_MODE_DEFAULT = 0,     /* battery default mode */
      BAT_MODE_NORMAL = 1,      /* battery normal mode */
      BAT_MODE_POWER_INDICAITON = 2,   /* battery power indicaiton mode */
      BAT_MODE_LOW_POWER = 3,   /* battery low power mode */
      BAT_MODE_FULL   = 4,      /* battery full */
   } T_BAT_MODE;

电量模式进入退出说明：

   1. BAT_MODE_DEFAULT：上电默认的模式，还没有进行过ADC采样，进行采样后变为其他模式。
   2. BAT_MODE_POWER_INDICAITON：电池电压低于阈值 BAT_ENTER_POWER_INDICATION_THRESHOLD，会进行LED闪烁提示，高于模式阈值退出该模式。
   3. BAT_MODE_LOW_POWER：电池电压低于 BAT_ENTER_LOW_POWER_THRESHOLD，鼠标进入低电量模式，关闭按键、滚轮、移动和灯效等所有功能，直到电池电压升到超过阈值或者插入USB充电后退出该模式。
   4. BAT_MODE_FULL：电池满电状态。
   5. BAT_MODE_NORMAL：除了上述模式外的其他状态。

灯效
-------

宏定义 **SUPPORT_LED_INDICATION_FEATURE** 设置为1时，打开LED功能。可以通过PAD输出高低电平直接控制LED，一般用于实现颜色和闪烁效果简单的灯效；也可以通过PWM输出控制LED，一般用于实现颜色和闪烁效果复杂且精确的灯效。通过宏 **LED_NUM_MAX** 设置LED的总个数。

PAD输出控制
~~~~~~~~~~~~~

宏 **LED_GPIO_CTL_NUM** 设置通过PAD输出控制的LED个数。RGB LED的管脚配置如下：

.. code-block:: c

   #define LED1_R_PIN                      P3_2
   #define LED1_G_PIN                      XO32K
   #define LED1_B_PIN                      XI32K
   static T_LED_PIN led_pin[LED_GPIO_CTL_NUM] =
   {
      {
         .rgb_led_pin[0] = LED1_R_PIN,
         .rgb_led_pin[1] = LED1_G_PIN,
         .rgb_led_pin[2] = LED1_B_PIN
      },
   };

PAD输出控制LED的原理如下：通过结构体 :mod:`T_LED_EVENT_STG` 记录每个LED闪烁事件的优先级，事件内循环次数，颜色，闪烁事件次数；通过 :func:`led_blink_start` 接口使能某个LED闪烁事件，并传入闪烁时间；通过 :func:`led_blink_exit` 接口停止某个LED闪烁事件；通过软件定时器 :mod:`led_gpio_ctrl_timer`，定时检查当前最高优先级的LED闪烁事件，当前时刻LED管脚所需要输出的电平，然后通过PAD SW mode设置正确的管脚电平。软件定时器 :mod:`led_gpio_ctrl_timer` 的超时时间为LED_PERIOD，默认为50ms，也就是每50ms检查和更改一次LED电平状态。

结构体 :mod:`T_LED_EVENT_STG` 如下：

.. code-block:: c

   typedef struct
   {
      uint8_t   led_type_index;
      uint8_t   led_loop_cnt;
      uint8_t   led_color_mask_bit[3];
      uint32_t  led_bit_map;
   } T_LED_EVENT_STG;


1. led_type_index：LED闪烁事件编号，也表征事件优先级，0为最高优先级，数值越大优先级越低。

2. led_loop_cnt：LED闪烁事件内循环次数，LED闪烁事件一次闪烁时间为软件定时器 :mod:`led_gpio_ctrl_timer` 超时时间 LED_PERIOD 乘以 led_loop_cnt。程序中 LED_PERIOD 默认为50ms，led_loop_cnt默认设置为20次，也就是LED闪烁事件一次闪烁时间为1s。

3. led_color_mask_bit[3]：LED颜色，可以配置为红，绿，蓝单一颜色或两两组合或三个组合。

4. led_bit_map：闪烁规律。在一次LED闪烁事件内，每个bit都表征一次软件定时器callback函数 :func:`led_gpio_ctrl_timer_cb` 调用时LED的亮灭状态。以第n个bit为例（用bit(n)表示），如果bit(n)为1，表示一次LED闪烁事件内，第n+1次调用 :func:`led_gpio_ctrl_timer_cb` 函数时，LED需处于亮的状态；如果bit(n)为0，则需要处于灭的状态。因为一次LED闪烁事件内只会调用 led_loop_cnt 次 :func:`led_gpio_ctrl_timer_cb` 函数，因此只有bit0到bit(led_loop_cnt-1)的值是有效的。

程序中默认的LED事件以及相关的结构体参数在数组 :mod:`led_event_arr` 中配置。

当调用 :func:`LED_BLINK` 即 :func:`led_blink_start` 时，触发某个闪烁事件。接口如下：

.. code-block:: c

   T_LED_RET_CAUSE led_blink_start(uint16_t led_index, LED_TYPE type, uint8_t cnt);

   #define LED_BLINK(led_index, type, n)   led_blink_start(led_index, type, n)

参数说明如下：

   1. led_index：LED Index，不能超过 LED_GPIO_CTL_NUM，程序默认只有一个LED_1（值为0）。
   2. type：LED闪烁事件。
   3. n：LED闪烁事件的执行次数，如果为0则表示无限次。

以 :mod:`LED_BLINK(LED_1, LED_TYPE_BLINK_RESET, 2)` 为例，表示LED_1执行闪烁事件LED_TYPE_BLINK_RESET 2次。该事件一次闪烁时间为 LED_PERIOD 乘以对应的 led_loop_cnt，为1s，且该事件闪烁规律是一直保持常亮，因此就实现了LED_1常亮2s的灯效。

PWM输出控制
~~~~~~~~~~~~~

PWM输出是通过硬件定时器实现的，一路PWM就需要配置一个硬件定时器（包含普通的HW Timer和Enhance Timer），因此一个RGB灯需要3个硬件定时器，目前程序使用的硬件定时器情况参考 :ref:`tri-mode mouse硬件定时器`，只有7个硬件定时器可以给LED模块使用。为了灯效变化的精准，还需要一个硬件定时器，定时检查是否需要改变灯效和PWM输出。因此只能使用7个硬件定时器，同时驱动2个RGB LED。在LED亮度允许的情况下，也可以通过分时复用的方式，一个硬件定时器分时控制两个管脚输出PWM，程序中并没有分时控制的实例，需要自行实现。

宏 **LED_HW_TIM_PWM_CTL_NUM** 设置通过PWM输出控制的LED个数。宏 **LED_HW_TIM_PWM_CTL_INDEX** 设置通过PWM输出控制的LED的起始index。LED的管脚和所使用的硬件定时器配置如下：

.. code-block:: c

   #define LED1_R_PIN                      P3_2
   #define LED1_G_PIN                      XO32K
   #define LED1_B_PIN                      XI32K

   typedef struct
   {
      uint8_t pin_num;
      uint8_t pin_func;
      TIM_TypeDef *timer_type;
      ENHTIM_TypeDef *entimer_type;
   } T_LED_PWM_PARAM_DEF;

   static const T_LED_PWM_PARAM_DEF led_pwm_list[3 * LED_HW_TIM_PWM_CTL_NUM] =
   {
      {LED2_R_PIN, TIMER_PWM2, TIM2, NULL    },
      {LED2_G_PIN, ENPWM2_P,   NULL, ENH_TIM2},
      {LED2_B_PIN, ENPWM3_P,   NULL, ENH_TIM3},
   };

PWM输出控制LED的原理如下：通过结构体 :mod:`T_PWM_LED_EVENT_STG` 记录每个LED闪烁事件的优先级，事件内循环次数，闪烁规律；通过 :func:`led_hw_tim_pwm_blink_start` 接口使能某个LED闪烁事件，并传入闪烁事件次数；通过 :func:`led_hw_tim_pwm_blink_exit` 接口停止某个LED闪烁事件；通过硬件定时器LED_CNT_TIM（默认设置的TIM6），定时检查并输出当前最高优先级的LED闪烁事件，当前时刻LED管脚所需要输出的PWM波形。硬件定时器 LED_CNT_TIM 的超时时间为LED_CNT_TIME_MS，默认为2ms，也就是每2ms检查和更改一次LED管脚的PWM波形。

结构体 :mod:`T_PWM_LED_EVENT_STG` 如下：

.. code-block:: c

   typedef struct
   {
      uint8_t   led_type_index;
      uint32_t  led_loop_num;
      LED_EVENT_HANDLE_FUNC led_event_handle_func;
   } T_PWM_LED_EVENT_STG;

1. led_type_index：LED闪烁事件编号，也表征事件优先级，0为最高优先级，数值越大优先级越低。

2. led_loop_num：LED闪烁事件内循环次数，LED闪烁事件一次闪烁时间为硬件定时器 LED_CNT_TIM 的超时时间为 LED_CNT_TIME_MS 乘以 led_loop_num。程序中 LED_CNT_TIME_MS 默认为2ms，led_loop_num默认设置为500次，也就是LED闪烁事件一次闪烁时间为1s。

3. led_event_handle_func：闪烁规律的callback函数，返回当前时刻LED的RGB颜色亮度参数。每次硬件定时器 LED_CNT_TIM 超时后，会调用该callback获取当前时刻LED事件颜色亮度参数。

程序中默认的LED事件以及相关的结构体参数在数组 :mod:`pwm_led_event_arr` 中配置：

.. code-block:: c

   static T_PWM_LED_EVENT_STG pwm_led_event_arr[PWM_LED_TYPE_MAX] =
   {
      {PWM_LED_TYPE_IDLE,        PWM_LED_LOOP_NUM_IDLE,      NULL},
      {PWM_LED_TYPE_BLINK_RESET, PWM_LED_LOOP_NUM_500_TIMES, led_reset_event_handler},
      {PWM_LED_TYPE_BLINK_TEST,  PWM_LED_LOOP_NUM_500_TIMES, led_test_event_handler},
      {PWM_LED_TYPE_ON,          PWM_LED_LOOP_NUM_500_TIMES, NULL},
   };

当调用 :mod:`PWM_LED_BLINK` 即 :mod:`led_hw_tim_pwm_blink_start` 时，触发某个闪烁事件。接口如下：

.. code-block:: c

   void led_hw_tim_pwm_blink_start(uint16_t led_index, PWM_LED_TYPE type, uint32_t cnt);

   #define PWM_LED_BLINK(led_index, type, cnt) led_hw_tim_pwm_blink_start(led_index, type, cnt)

参数说明如下：

   1. led_index：LED Index，不能超过 LED_HW_TIM_PWM_CTL_INDEX + LED_HW_TIM_PWM_CTL_NUM。
   2. type：LED闪烁事件。
   3. n：LED闪烁事件的执行次数，如果为0则表示无限次。

看门狗
---------

RTL87x2G平台有两种看门狗，可以分别在 CPU active 和 DLPS 状态下工作，在 :file:`board.h` 中设置 **WATCH_DOG_ENABLE** 为1时将看门狗都打开。通过宏定义 **WATCH_DOG_TIMEOUT_MS** 修改watchdog timeout时间（默认设置的是5秒），喂狗时间是（WATCH_DOG_TIMEOUT_MS - 1）秒，超过时间没有喂狗会自动重启。打开看门狗后，会开一个软件定时器，每隔一段时间（小于超时时间）定时喂狗。

可以调用接口 :func:`app_system_reset`，主动触发看门狗重启，可以配置重启的模式，包括 RESET_ALL 和 RESET_ALL_EXCEPT_AON。可以记录重启的原因，在IC重启上电后可以获取重启原因。用户可以添加新的原因至 :mod:`T_SW_RESET_REASON` 中。

DFU
------

本文中DFU（Device Firmware Upgrade）是指设备的固件升级，基于USB来实现，需要将 :file:`board.h` 中宏定义 **FEATURE_SUPPORT_USB_DFU** 置1来打开该功能。其原理是，通过 MPPackTool 将想要升级的image进行打包，而后通过 CFUDownloadTool 将打包的image内容通过USB传输给鼠标，鼠标将需要升级的image存储到IC的OTA Temp区，当数据传输完成并校验通过，会自动重启将OTA Temp区所有image搬运到Flash的运行区域，以实现固件升级。

DFU支持一次打包并升级一个或多个image，可以打包并升级以下5个image：:file:`boot patch`、:file:`system patch`、:file:`host image`、:file:`stack patch`、:file:`app image`。但一次打包的image大小总和（不包括 :file:`boot patch`）不能超过OTA Temp区，以默认的flash map为例，app image和host image都较大，不能一起打包，五份image需分为两个升级包。

DFU数据传输完成后，需要把升级image从OTA Temp区搬运到Flash运行区，当该过程中断电重启后，会重新搬运，IC不会变砖。

DFU升级单独使用了一个USB interface，基于HID set/get report来实现升级过程的数据交互，具体实现参考 :file:`usb_hid_interface_dfu.c/.h`。

Tri-Mode Mouse application遵循 \ :Doc:`CFU V1升级协议 <../../../../subsys/cfu/doc/text_cn/CFU_V1>`\ ，具体实现可参考 :file:`usb_dfu.c/.h`。

RTL87x2G mouse DFU升级，打包工具的使用可参考 :ref:`tri-mode mouse MPPack Tool`。升级时，只需要把设备通过USB接入电脑，升级工具的使用可参考 :ref:`tri-mode mouse CFUDownloadTool`。

低功耗
---------

DLPS
~~~~~~~~

DLPS功能，是当CPU和外设不需要工作时，关闭相关模块电源，以达到省电的目的。在 :file:`board.h` 中配置宏定义 **DLPS_EN** 为1，打开DLPS功能。

宏定义 **USE_USER_DEFINE_DLPS_EXIT_CB** 和 **USE_USER_DEFINE_DLPS_ENTER_CB** 设置为1，在退出DLPS和进入DLPS时调用callback进行应用需要的配置。

打开相关宏定义，在进入DLPS的时候会自动保存对应模块的寄存器，并在退出DLPS的时候恢复寄存器配置。以GPIO为例，将 USE_GPIOA_DLPS 和 USE_GPIOB_DLPS 配置为1后，会在进出DLPG时自动保存和恢复GPIO相关寄存器配置，这样退出DLPS时，不需要重新初始化GPIO。

DLPS更详细的说明，参考文档 \ :Doc:`低功耗模式 <../../../../doc/platform/low_power_mode/text_cn/README>`\。

进入DLPS前配置
^^^^^^^^^^^^^^^^^

由于进入DLPS后大多数外设会断电，有些外设对应的管脚有外围电路，需要配置相关管脚的电平防止外围电路漏电。 **USE_USER_DEFINE_DLPS_ENTER_CB** 设置为1，注册了进入DLPS前调用的app callback函数 :func:`app_enter_dlps`，函数中会对各个模块的管脚进行PAD配置，将管脚PAD设置为PAD_SW_MODE，配置为需要的输入输出模式和电平。有些功能的管脚需要能够唤醒系统退出DLPS，比如按键功能，可以在进入DLPS前在app callback中，调用 :func:`System_WakeUpPinEnable` 使能对应管脚PAD唤醒DLPS的功能，可以配置唤醒电平极性和debounce时间。DLPS PAD唤醒的debounce时间是所有管脚共用的，因此程序中并没有使能PAD debounce，而是每个管脚唤醒后用对应模块自己的debounce或者sw timer debounce。

进入dlps时调用的app callback。

.. code-block:: c

   static void app_enter_dlps(void)
   {
      Pad_ClearAllWakeupINT();
      System_WakeupDebounceStatus(0);
   #if MOUSE_GPIO_BUTTON_EN
      mouse_gpio_button_module_enter_dlps_config();
   #elif MOUSE_KEYSCAN_EN
      mouse_keyscan_module_enter_dlps_config();
   #endif
   #if AON_QDEC_EN
      qdec_module_enter_dlps_config();
   #endif
   #if GPIO_QDEC_EN
      gpio_qdec_module_enter_dlps_config();
   #endif
   #if PAW3395_SENSOR_EN
      paw3395_module_enter_dlps_config();
   #endif
   #if MODE_MONITOR_EN
      mode_monitor_module_enter_dlps_config();
   #endif
   #if SUPPORT_BAT_DETECT_FEATURE
      bat_enter_dlps_config();
   #endif
   #if (WATCH_DOG_ENABLE == 1)
      if (app_global_data.is_aon_wdg_enable == true)
      {
         AON_WDT_Start(AON_WDT, WATCH_DOG_TIMEOUT_MS, RESET_ALL_EXCEPT_AON);
      }
   #endif
   }

退出DLPS后配置
^^^^^^^^^^^^^^^^

退出DLPS后，需要将进入DLPS前所配置的管脚重新初始化。 **USE_USER_DEFINE_DLPS_EXIT_CB** 设置为1，注册了退出DLPS后调用的app callback函数 :func:`app_exit_dlps`，函数中会对各个模块的管脚进行PAD配置，将管脚PAD设置为pinmux mode，并且映射到对应的外设功能。

退出dlps时调用的app callback。

.. code-block:: c

   static void app_exit_dlps(void)
   {
   #if MOUSE_GPIO_BUTTON_EN
      mouse_gpio_button_module_exit_dlps_config();
   #elif MOUSE_KEYSCAN_EN
      mouse_keyscan_module_exit_dlps_config();
   #endif
   #if PAW3395_SENSOR_EN
      paw3395_module_exit_dlps_config();
   #endif
   #if MODE_MONITOR_EN
      mode_monitor_module_exit_dlps_config();
   #endif
   #if SUPPORT_BAT_DETECT_FEATURE
      bat_exit_dlps_config();
   #endif
   #if GPIO_QDEC_EN
      gpio_qdec_module_exit_dlps_config();
   #endif
   #if (WATCH_DOG_ENABLE == 1)
      if (app_global_data.is_aon_wdg_enable == true)
      {
         AON_WDT_Disable(AON_WDT);
      }
   #endif
   }

CPU WFI
~~~~~~~~~~

当外设需要工作，比如鼠标一直移动时，是无法进入DLPS的，此时CPU可能不需要工作，可以进入WFI来省电。正常情况，CPU进入WFI需要耗费一些检查的时间，为了降低功耗，可以在确定不能进入DLPS的场景下，调用接口 :func:`pm_no_check_status_before_enter_wfi`，使CPU能更快进入WFI，但调用该接口后，无法再进入DLPS；当需要进入DLPS时，需要调用 :func:`pm_check_status_before_enter_wfi_or_dlps`，而后可以正常进入DLPS和CPU WFI。

程序中默认在以下几种情况下调用 :func:`pm_no_check_status_before_enter_wfi`。

   - 鼠标移动，光学传感器定时采样时。
   - 鼠标处于USB模式下，且USB不在suspend状态。

程序默认在以下几种情况下调用 :func:`pm_check_status_before_enter_wfi_or_dlps`。

   -  鼠标停止移动，停止光学传感器定时采样时。
   -  鼠标处于USB模式下，USB进入suspend状态。
   -  鼠标电量模式处于 BAT_MODE_LOW_POWER 时。

无操作断线和休眠
~~~~~~~~~~~~~~~~~~

当宏定义 **FEATURE_SUPPORT_NO_ACTION_DISCONN** 设置为1时（默认为1），打开无操作断线功能，鼠标处于BLE或2.4G模式下，不使用持续超过一定时间后，会主动断开连接来省电，此时按键、滚轮和移动鼠标均可以重新连接和使用。无操作断线时间可以通过宏定义 **NO_ACTION_DISCON_TIMEOUT** 修改，默认设置为1分钟。

当宏定义 **FEATURE_SUPPORT_NO_ACTION_DISCONN** 和 **FEATURE_SUPPORT_NO_ACTION_SENSOR_SLEEP** 都设置为1时（默认为1），打开无操作断线功能的同时，也会打开无操作休眠的功能。鼠标处于BLE或2.4G模式下，且在断线状态，无操作超过一定时间，会将一些漏电的模块关闭，来进一步省电，此时只能通过有限的手段唤醒鼠标重新使用，默认是关闭了光学传感器，无法通过移动鼠标唤醒，只能通过按键和滚轮唤醒鼠标重新使用。无操作休眠时间可以通过宏定义 **NO_ACTION_SENSOR_SLEEP_TIMEOUT** 修改，默认设置为1分钟。

FTL
------

FTL 是提供给BT stack 和APP 用户读写flash 上数据的抽象层，通过FTL 接口可以通过逻辑地址直接读写flash 上分配出FTL 空间的数据，操作更为简便。对于鼠标应用而言，建议将小数据，或经常改写的数据存到FTL中，方便操作，且FTL操作不会有erase flash行为（erase flash过程会屏蔽所有中断）。FTL有两种版本，v1和v2，目前鼠标应用采用的是v2版本，详情请参考文档 \ :Doc:`Memory <../../../../doc/platform/memory/text_cn/README>`\。

初始化
~~~~~~~~

FTL v2版本需要app进行FTL分模块进行初始化，SDK中，app只使用了一个模块，其在main函数中进行了初始化，最小单位设置为4 byte，分配了1016 byte：:mod:`ftl_init_module("app", 0x3F8, 4)`。

FTL初始化能分配的逻辑地址大小，取决于flash map中FTL物理地址区域的大小，其对应关系和调整的依据和方法，参考文档 \ :Doc:`Memory <../../../../doc/platform/memory/text_cn/README>`\。

空间规划和调整
~~~~~~~~~~~~~~~~

在 :file:`board.h` 中有规划目前app所使用的FTL空间。其中前16 byte是留给2.4G保存配对信息用的，app能使用的地址是[16，1016]。SDK中将DPI，上报率，蓝牙的配对信息备份，蓝牙随机地址保存在FTL区域，用户可以自行调整现有数据的地址，或规划添加新数据，保证存储的数据不重叠即可。

垃圾回收
~~~~~~~~~~

当FTL物理空间将要被写满时，会自动进行垃圾数据的回收，释放物理空间。进行垃圾回收的过程，会有erase flash行为，会屏蔽所有中断。为了尽可能避免垃圾回收对正常程序行为的影响，可以将宏 **FEATURE_SUPPORT_APP_ACTIVE_FTL_GC** 置1，app在鼠标不使用的时候主动进行垃圾回连，垃圾回收的触发变的可控。

主动触发垃圾回收的场景有：

   1. BLE模式下断线。
   2. 2.4G模式下断线，且一段时间回连不上。
   3. 无操作主动断线后。

产测
-------

产测模式的进入退出方式
~~~~~~~~~~~~~~~~~~~~~~~

通过GPIO触发进入
^^^^^^^^^^^^^^^^^^

:file:`board.h` 中宏 **THE_WAY_TO_ENTER_MP_TEST_MODE** 配置为 ENTER_MP_TEST_MODE_BY_GPIO_TRIGGER 时，上电检查 MP_TEST_PIN_1 和 MP_TEST_PIN_2 两个管脚电平，电平满足条件就进入产测模式。在 :func:`mp_test_mode_check_and_enter` 函数中调整进入产测模式的两个电平状态。

通过USB命令进入退出
^^^^^^^^^^^^^^^^^^^^^

:file:`board.h` 中宏 **THE_WAY_TO_ENTER_MP_TEST_MODE** 配置为 ENTER_MP_TEST_MODE_BY_USB_CMD 时，可以通过USB命令来进入退出产测模式。已经默认实现的功能如下。

Set report相关命令，在 :func:`mp_test_set_report_handle` 函数中实现：

   1. 进入产测模式：:mod:`14(report id)` :mod:`56` :mod:`43` :mod:`54` :mod:`65` :mod:`73` :mod:`74`。
   2. 进入/退出single tone测试模式：:mod:`14(report id)` :mod:`56` :mod:`43` :mod:`01` :mod:`78` :mod:`fc` :mod:`04` :mod:`on_off_state` :mod:`channel` :mod:`tx_power`。

      - on_off_state：1 - 进入single tone test mode，0 - 退出single tone test mode
      - channel：实际频点为2402 + channel(MHz)
      - tx_power：8:4dbm, 0:0dbm, -8:-4dbm, -14:-7dbm, -38:-19dbm

   3. 临时调频偏（不会保存，重启后恢复）：:mod:`14(report id)` :mod:`56` :mod:`43` :mod:`58` :mod:`74` :mod:`61` :mod:`6C` :mod:`00` :mod:`xtal_value`。
   4. 将频偏值写入config file中：:mod:`14(report id)` :mod:`56` :mod:`43` :mod:`58` :mod:`74` :mod:`61` :mod:`6C` :mod:`01` :mod:`xtal_value`。
   5. 重启回到正常模式：:mod:`14(report id)` :mod:`56` :mod:`43` :mod:`52` :mod:`65` :mod:`73` :mod:`65` :mod:`74`。

Get report可以获取当前频偏值，在 :func:`mp_test_get_report_handle` 函数中实现：:mod:`14 xi xo 00 00 ...(xi = xo = xtal_value)`。

Single tone实现方式
~~~~~~~~~~~~~~~~~~~~~~~

通过BLE GAP层实现
^^^^^^^^^^^^^^^^^^^^

:file:`board.h` 中宏 **MP_TEST_SINGLE_TONE_MODE** 配置为 GAP_LAYER_SINGLE_TONE_INTERFACE 时，上电需要进行BLE gap初始化，在gap ready一段时间（100ms）后，才能调用接口 :func:`single_tone` 进入/退出single tone test mode以及配置single tone参数。推荐使用该方式，该方式在初始化完成后，app层的调用非常简单。

通过HCI层实现
^^^^^^^^^^^^^^^

:file:`board.h` 中宏 **MP_TEST_SINGLE_TONE_MODE** 配置为 HCI_LAYER_SINGLE_TONE_INTERFACE 时，上电不需要进行BLE gap初始化，直接通过HCI命令控制single tone。但是初始化额外的os task，以及注册一些callback对event进行操作。

快速配对模式
~~~~~~~~~~~~~~

快速配对（以下简称快连）实现了产线上快速测试样机的无线功能，包括BLE和2.4G。以下提供两种方案：

   1. 方案一（固定地址快连）：dongle和鼠标使用固定地址生成配对信息后进行快速连接。

   2. 方案二（非固定地址快连）：鼠标端通过组合键触发 BLE/2.4G 广播（无需修改代码），dongle处于非连接状态时，保持2.4G/BLE扫描，连接任意符合条件的鼠标。

固定地址快连
^^^^^^^^^^^^^^

使用方案一时，鼠标端需要把 :file:`board.h` 中的宏 **FAST_PAIR_TEST_MODE** 置1，dongle端需要把宏 **FAST_PAIR_TEST_MODE** 、宏 **SUPPORT_SET_BOND_INFO_USED_BY_FAST_PAIR** 都置1。

   - 2.4G模式下，dongle和鼠标都会预先设定地址并生成配对信息，当rssi强度大于APP设定的阈值时，两者才会成功连接，阈值可以通过宏 **PPT_PAIR_RSSI_THRESHOLD** 进行修改。公版SDK准备了5组配对地址，可供5条产线同时工作。鼠标通过不同的组合键可选择使用哪一组地址；而dongle可通过set report下USB指令选择地址，指令已在 :func:`mp_test_get_report_handle` 函数中实现：:mod:`14(report id)` :mod:`55` :mod:`65` :mod:`75` :mod:`(bond_info_index)`。

   - BLE模式使用public地址进行快连，流程与2.4G模式类似，区别在于:
   
      1. 连接条件：BLE模式使用固定地址快连时无需判断rssi强度，但会判断扫描到的地址，通过回连流程实现快连功能。
      2. 如果宏 **CLEAR_BOND_INFO_WHEN_FACTORY_TEST** 置1，当鼠标和dongle成功连接后，dongle端会发出指令使鼠标清除配对信息。
      3. dongle同样可通过set report下USB指令选择配对地址，指令有所不同：:mod:`14(report id)` :mod:`55` :mod:`65` :mod:`74` :mod:`(bond_info_index)`。

   .. note:: 
      在BLE模式下使用固定地址快连，会涉及修改config文件，必须确保电源稳定，否则会有概率变砖。

非固定地址快连
^^^^^^^^^^^^^^^^

使用方案二时，鼠标按下组合键 :kbd:`左键+滚轮中键+右键` 发 BLE/2.4G 广播（无需修改代码）。dongle端需要把 :file:`board.h` 中的宏 **FAST_PAIR_TEST_MODE** 置1、宏 **SUPPORT_SET_BOND_INFO_USED_BY_FAST_PAIR** 设为0，通过set report下USB指令进入快连模式，USB指令与方案一相同。

   - 2.4G模式下，与常规配对流程一致，唯一区别是，当收到 SYNC_EVENT_CONNECT_LOST 超过1s，dongle会清除配对信息重新进入配对状态。

   - BLE模式下，dongle通过检查鼠标广播的内容和rssi，可以连接任何符合要求的设备，并通过HID指令清除配对信息。rssi阈值可以通过dongle端的宏 **BLE_FAST_PAIR_RSSI_THRESHOLD** 进行修改。如果用户需要适配不同的鼠标，dongle端的广播过滤内容要和鼠标保持一致。