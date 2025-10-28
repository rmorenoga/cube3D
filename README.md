https://github.com/cellularbricks/cellularbricks.github.io/raw/refs/heads/main/cellular_brick_banner_wide.mp4

# Smart Cellular Bricks for Decentralized Shape Classification and Damage Recovery

## System Requirements
### Hardware requirements
Training, simulation and plot generation are setup as a Python jupyter notebook **BIGCUBE_V2.ipynb**. We reccomend Google [colab](https://colab.research.google.com/) to run it. To open the notebook in colab you can either clone this repository and upload the notebook to Google colab, or create a fork of the repository and open it directly from github using Google colab. Most cells run on the free version of colab, however, for training you should use one of the paid plans as it uses resources over the free limits (12 GB of RAM).

Firmware compilation and communication with the cubes has been tested on a standard computer running Windows 10 or 11. See **Cube Firmware** below. 

The electronics folder contains the electronic designs necessary to build a cube in order to run the classification code in the robots. 

### Software requirements
Follow the instructions in the jupyter notebook to install the required Python packages and run the different cells. The notebook runs on the 2025.07 runtime version of colab (see more information [here](https://research.google.com/colaboratory/runtime-version-faq.html#2025.07)). Other Python packages required include (See **Install all required python packages** inside the notebook):
- keras==2.11.0
- tensorflow==2.12.0
- pygifsicle

and the **gifsicle** program which can be installed by calling in a cell:
!sudo apt-get install gifsicle

Uploading the firmware to the cubes uses Python scripts, the requirements.txt in /OTAPython/ lists the necessary dependencies.
Getting data from the cubes requires installing [docker](docker.com/) in your system.

Both functions have been tested in a standard computer with Windows 11. See further details in each section below.


# Installation instructions 

Training and simulation are performed using python code organized in a jupyter notebook that runs preferably in Google colab **BIGCUBE_V2.ipynb**. To open it you can either clone this repository and upload the notebook to Google colab, or create a fork of the repository and open it directly from github using Google colab. The notebook also contains functions for exporting the model weights as a file for compiling the microcontroller code, tools for checking connections between real cubes and tools for generating graphs. For further instructions check the different cell descriptions inside the notebook. 

## Outputting neural network for firmware
To output the neural network to be uploaded to the firmware, run the corresponding cell in the jupyter notebook. It produces a text file that can be copied directly into the neural_network.h required by the firmware.

## Cube firmware
The cube firmware is saved in the 3DCubePlatformIO folder. You also need the neural_network.h file located in the root file of the repository. The following steps have been tested on Windows 10 and 11.

### Compiling
To be able to upload the firmware to the cubes it should first be compiled. The 3DCubePlatformIO folder is set up and configured as a [platformIO](platformio.org/) project. To make things easier we reccomend opening the folder with [visual studio code](https://code.visualstudio.com/). VSCode will automatically detect the platformio.ini file in the repository and ask you to install the platformIO extension. The extension will then automatically donwload all the required compilers and libraries. After installation press the compile button (check mark at the bottom left corner of VSCode). This will generate the firmware.bin file in the 3DCubePlatformIO/.pio/build/adafruit_metro_esp32s2/ subfolder. 

### Uploading
To upload the firmware.bin file first make sure the cubes are connected to the same local network. Then navigate to the OTApython folder and run the ESPuploadParallel.py script. This script should upload the firmware to all cubes connected in the network. To change the subnet that is scanned by the script, change the address ranges in lines 18, 80, 86, 52 and 53.

### Getting data from the cubes

After all cubes have been programmed. Turn them off, navigate to the StateReceivers folder in a terminal and launch the following command:

	docker compose up

 This will run the server configured in **docker-compose.yaml**. This requires that you have [docker](docker.com/) installed in your system and assumes that you have a [mongodb](mongodb.com/) installation with a **Test** database and a **Dummy** collection set up. Additionally make sure that the computer where you launch the server is on IP 192.168.72.22 and that you have firewall permissions to receive on TCP port 8000. You can change the server address in the firmware (/3DCubePlatformIO/src/3d_cube.ino) in line 688, this needs the cubes to be reprogrammed. After you launch the server, turn on the assembled cubes. The cubes will first perform the classification process, then connect to the network specified in lines 602 and 603 in the firmware (/3DCubePlatformIO/src/3d_cube.ino) and then post the data to the server.

After the data from all cubes arrive to the server, you can export the collection as a .json file.

## Electronics
This folder contains the electronic PCB designs necessary to fabricate a cube.

## Training Data:

The training data is downloaded from https://www.shapenet.org/ as ModelShapeNetCore, model_normalized.solid.binvox format. We use 7 classes: aircraft, boat, car, guitar, birdhouse(house), table and chair.

This data is then reduced by the function “training_data_cleaning.py”. Running this file will present you with scaled down versions of the downloaded training data. If you think the image is acceptable, close the image popup, and write “y” into the terminal. Else, write “n” in the terminal. The terminal will display how many successful images have been saved and also the number of voxels in each piece of data. Note: the data is expected to be in the file subfolder location vox_data/…. Cleaning the data is done one class at a time, and labels are saved simultaneously with the xdata. To change the class type, change line 129. 

The classification output should be as follows:

**N.  Class   Color**
- 0 **Plane** green  
- 1 **Chair** blue  
- 2 **Car** purple  
- 3 **Table** yellow 
- 4 **House** white  
- 5 **Guitar** pink  
- 6 **Boat** cyan
- **Damaged** red


The code will output two .npy files xdata and ydata that are ready to be inputted to the neural network training code. **Already cleaned data is stored in the xdata_7class.npy and ydata_7class.npy**

## Training for damage detection and recovery

Our code for damage detection and recovery can be found here: https://github.com/shyamsn97/cube-regen



 

